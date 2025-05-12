#include "raylib.h"
#include "raymath.h"
#include "array.h"
#include "dda.h"

const int screenWidth = 1280;
const int screenHeight = 720;

#define CHUNK_SIZE 64
int world[CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE] = { 0 };

Mesh worldMesh = { 0 };
Camera camera = { (Vector3){ 10.0f, 10.0f, 30.0f }, (Vector3){ CHUNK_SIZE / 2, 5.0f, CHUNK_SIZE / 2 }, (Vector3){ 0.0f, 1.0f, 0.0f }, 60.0f, CAMERA_PERSPECTIVE };

int currentBlock = 1;
bool isMenuOpen = false;
const int textureGridSize = 16;
const int textureSize = 16;
const int scale = 2;
const int scaledTextureSize = textureSize * scale;

void ReloadMesh();
void PlaceBreakBlock(Model model);
void SaveWorld();
void LoadWorld();
void DrawTextureMenu(Texture2D textureAtlas);
float SignedDistanceFunction(Vector3 point);
void UpdatePlayer(float deltaTime);
void UpdateHotbarSelection();
void DrawHotbar(Texture texture, Texture other);

typedef struct {
    Vector3 position;
    Vector3 velocity;
    float yaw;
    float pitch;
} Player;

Player player = { 0 };
const float GRAVITY = -9.8f;
const float JUMP_FORCE = 6.0f;
const float PLAYER_SPEED = 8.0f;
const float MOUSE_SENSITIVITY = 0.003f;
const float PLAYER_RADIUS = 0.3f;

int breakingID = -1;
float breakingTime = 0.0f;
Texture2D animations[10] = {0};

#define HOTBAR_SIZE 9

int hotbar[HOTBAR_SIZE] = { 1, 2, 3, 4, -5, -4, -3, -2, -1 };
int selectedHotbarIndex = 0;

int main() {
    SetConfigFlags(FLAG_VSYNC_HINT | FLAG_MSAA_4X_HINT);
    InitWindow(screenWidth, screenHeight, "freakyKraft 2");
    DisableCursor();

    LoadWorld();
    ReloadMesh();
    Shader shader = LoadShader("shaders/vertex.glsl","shaders/fragment.glsl");
    shader.locs[SHADER_LOC_VECTOR_VIEW] = GetShaderLocation(shader, "viewPos");
    Texture2D texture = LoadTexture("atlas.png");
    Material material = LoadMaterialDefault();
    material.maps[MATERIAL_MAP_DIFFUSE].texture = texture;
    material.shader = shader;

    Mesh mesh = GenMeshCube(1.01f, 1.01f, 1.01f);
    Model model = LoadModelFromMesh(mesh);
    Image img = LoadImage("breaking.png");
    Texture2D otherTexture = LoadTexture("other.png");

    for (int i = 0; i < 10; i++) {
        Image imgCpy = ImageCopy(img);
        ImageCrop(&imgCpy, (Rectangle) {i * 16, 0, 16, 16});
        animations[i] = LoadTextureFromImage(imgCpy);
    }

    player.position = (Vector3){ 16, 10.0f, CHUNK_SIZE / 2.0f };
    player.velocity = (Vector3){ 0.0f, 0.0f, 0.0f };
    player.yaw = 0.0f;
    player.pitch = 0.0f;

    while (!WindowShouldClose()) {
        float deltaTime = GetFrameTime();

        if (IsKeyPressed(KEY_N)) {
            SaveWorld();
        }

        if (IsKeyPressed(KEY_M)) {
            LoadWorld();
        }

        if (IsKeyPressed(KEY_TAB)) {
            isMenuOpen = !isMenuOpen;
            if(isMenuOpen) EnableCursor();
            else DisableCursor();
        }

        if (!isMenuOpen) {
            UpdatePlayer(deltaTime);
        }

        SetShaderValue(shader, shader.locs[SHADER_LOC_VECTOR_VIEW], &camera.position.x, SHADER_UNIFORM_VEC3);

        BeginDrawing();
        ClearBackground((Color){ 64, 180, 255 , 255});
        BeginMode3D(camera);
        DrawMesh(worldMesh, material, MatrixIdentity());

        if(!isMenuOpen) {
            PlaceBreakBlock(model);
        }

        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int y = 0; y < CHUNK_SIZE; y++) {
                for (int z = 0; z < CHUNK_SIZE; z++) {
                    int id = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
                    int blockId = world[id] - 1;
                    if(blockId >= -1) continue;

                    int texX = (blockId % 8) * textureSize;
                    int texY = (blockId / 8) * textureSize;
                    Rectangle source = { texX, texY, textureSize, textureSize };

                    DrawBillboardPro(camera, otherTexture, source, (Vector3){x + 0.5, y + 0.5, z + 0.5},  (Vector3){0, 1, 0}, (Vector2){1, 1}, (Vector2){0.5, 0.5}, 0, WHITE);
                }
            }
        }

        EndMode3D();
        DrawFPS(1195, 5);
        DrawLineEx((Vector2){screenWidth / 2 - 10, screenHeight / 2}, (Vector2){screenWidth / 2 + 10, screenHeight / 2}, 2, WHITE);
        DrawLineEx((Vector2){screenWidth / 2, screenHeight / 2 - 10}, (Vector2){screenWidth / 2, screenHeight / 2 + 10}, 2, WHITE);

        if(isMenuOpen) {
            DrawTextureMenu(texture);
        } else {
            UpdateHotbarSelection();
            DrawHotbar(texture, otherTexture);
        }

        EndDrawing();
    }

    CloseWindow();
}


void UpdateHotbarSelection() {
    int wheelMove = GetMouseWheelMove();
    if (wheelMove != 0) {
        selectedHotbarIndex = (selectedHotbarIndex - wheelMove + HOTBAR_SIZE) % HOTBAR_SIZE;
        currentBlock = hotbar[selectedHotbarIndex];
    }
}

void DrawHotbar(Texture texture, Texture other) {
    int hotbarWidth = HOTBAR_SIZE * 70 + 15;
    int hotbarHeight = 80;
    int xPos = (screenWidth - hotbarWidth) / 2;
    int yPos = screenHeight - 80;

    Rectangle bgRect = { xPos, yPos, hotbarWidth, hotbarHeight };
    DrawRectangleRec(bgRect, (Color){ 0, 0, 0, 150 });

    for (int i = 0; i < HOTBAR_SIZE; i++) {
        int blockID = (hotbar[i] - 1);
        Texture tex = blockID >= 0 ? texture : other;

        int slotX = xPos + 10 + i * 70;
        Rectangle slotRect = { slotX, yPos + 10, 64, 64 };

        int atlasCols = blockID > 0 ? 16 : 8;
        int texX = (blockID % atlasCols) * textureSize;
        int texY = (blockID / atlasCols) * textureSize;
        Rectangle source = { texX, texY, textureSize, textureSize };
        DrawTexturePro(tex, source, slotRect, (Vector2){0, 0}, 0.0f, WHITE);

        Color borderColor = (i == selectedHotbarIndex) ? WHITE : GRAY;
        DrawRectangleLinesEx(slotRect, 2, borderColor);
    }
}

float SignedDistanceFunction(Vector3 point) {
    float minDistance = INFINITY;

    int x0 = (int)floorf(point.x);
    int y0 = (int)floorf(point.y);
    int z0 = (int)floorf(point.z);

    for (int dx = -1; dx <= 1; dx++) {
        for (int dy = -1; dy <= 1; dy++) {
            for (int dz = -1; dz <= 1; dz++) {
                int x = x0 + dx;
                int y = y0 + dy;
                int z = z0 + dz;

                if (x >= 0 && x < CHUNK_SIZE && y >= 0 && y < CHUNK_SIZE && z >= 0 && z < CHUNK_SIZE) {
                    if (world[x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE] > 0) {
                        Vector3 closest = {
                            fmaxf(x, fminf(point.x, x + 1)),
                            fmaxf(y, fminf(point.y, y + 1)),
                            fmaxf(z, fminf(point.z, z + 1))
                        };
                        float distance = Vector3Distance(point, closest);
                        minDistance = fminf(minDistance, distance);
                    }
                }
            }
        }
    }

    return minDistance;
}

void UpdatePlayer(float deltaTime) {
    player.velocity.y += GRAVITY * deltaTime;

    Vector3 moveDirection = { 0 };
    if (IsKeyDown(KEY_W)) moveDirection.z += 1.0f;
    if (IsKeyDown(KEY_S)) moveDirection.z -= 1.0f;
    if (IsKeyDown(KEY_A)) moveDirection.x -= 1.0f;
    if (IsKeyDown(KEY_D)) moveDirection.x += 1.0f;

    Vector3 forward = { cosf(player.yaw), 0, sinf(player.yaw) };
    Vector3 right = { -sinf(player.yaw), 0, cosf(player.yaw) };

    Vector3 movement = Vector3Add(
        Vector3Scale(forward, moveDirection.z),
        Vector3Scale(right, moveDirection.x)
    );

    movement.y = 0;
    if(Vector3Length(movement) > 0) {
        movement = Vector3Scale(Vector3Normalize(movement), PLAYER_SPEED * deltaTime);
    }

    if (IsKeyPressed(KEY_SPACE) && SignedDistanceFunction((Vector3){player.position.x, player.position.y - PLAYER_RADIUS, player.position.z}) < 0.01f) {
        player.velocity.y = JUMP_FORCE;
    }

    player.position = Vector3Add(player.position, (Vector3){movement.x, 0, movement.z});
    float distance = SignedDistanceFunction(player.position);

    if (distance < PLAYER_RADIUS) {
        Vector3 normal = {
            SignedDistanceFunction((Vector3){player.position.x + 0.01f, player.position.y, player.position.z}) - distance,
            0,
            SignedDistanceFunction((Vector3){player.position.x, player.position.y, player.position.z + 0.01f}) - distance
        };

        normal = Vector3Normalize(normal);
        player.position = Vector3Add(player.position, Vector3Scale(normal, PLAYER_RADIUS - distance));
    }

    float oldY = player.position.y;
    player.position.y += player.velocity.y * deltaTime;
    distance = SignedDistanceFunction((Vector3){player.position.x, player.position.y - 0.1f, player.position.z});

    if (distance < 0.2f) {
        if (player.velocity.y < 0) {
            player.position.y = oldY;
            player.velocity.y = 0;
        } else {
            player.position.y = player.position.y + (0.2f - distance);
            player.velocity.y = 0;
        }
    }

    distance = SignedDistanceFunction((Vector3){player.position.x, player.position.y + 1.4f, player.position.z});

    if (distance < 0.2f) {
        if (player.velocity.y > 0) {
            player.velocity.y = 0;
        }
    }

    Vector2 mouseDelta = GetMouseDelta();
    player.yaw += mouseDelta.x * MOUSE_SENSITIVITY;
    player.pitch -= mouseDelta.y * MOUSE_SENSITIVITY;
    player.pitch = Clamp(player.pitch, -PI/2 + 0.01f, PI/2 - 0.01f);

    camera.position = Vector3Add(player.position, (Vector3){ 0, 1.4f, 0 });
    camera.target = Vector3Add(camera.position, (Vector3){
        cosf(player.yaw) * cosf(player.pitch),
        sinf(player.pitch),
        sinf(player.yaw) * cosf(player.pitch)
    });
}

void DrawTextureMenu(Texture2D textureAtlas) {
    int atlasCols = textureAtlas.width / textureSize;
    int atlasRows = textureAtlas.height / textureSize;
    int startX = (screenWidth - textureGridSize * scaledTextureSize) / 2;
    int startY = (screenHeight - textureGridSize * scaledTextureSize) / 2;

    for (int y = 0; y < textureGridSize; y++) {
        for (int x = 0; x < textureGridSize; x++) {
            int type = y * textureGridSize + x;

            if (type >= atlasCols * atlasRows) continue;

            int texX = (type % atlasCols) * textureSize;
            int texY = (type / atlasCols) * textureSize;

            Rectangle source = { texX, texY, textureSize, textureSize };
            Rectangle dest = { startX + x * scaledTextureSize, startY + y * scaledTextureSize, scaledTextureSize, scaledTextureSize };
            DrawTexturePro(textureAtlas, source, dest, (Vector2){0, 0}, 0.0f, WHITE);

            if (IsMouseButtonPressed(MOUSE_LEFT_BUTTON) && CheckCollisionPointRec(GetMousePosition(), dest)) {
                currentBlock = type + 1;
                hotbar[selectedHotbarIndex] = currentBlock;

                isMenuOpen = false;
            }
        }
    }

    if(!isMenuOpen) {
        DisableCursor();
    }
}

void PlaceBreakBlock(Model model) {
    DDACursor cursor = DDACursorCreate(camera.position, Vector3Normalize(Vector3Subtract(camera.target, camera.position)));

    int i = 0;
    for (i = 0; i < 12; i++) {
        Vector3 mapPos = DDACursorStep(&cursor);

        int blockX = (int)floor(mapPos.x);
        int blockY = (int)floor(mapPos.y);
        int blockZ = (int)floor(mapPos.z);
        int blockID = blockX + blockY * CHUNK_SIZE + blockZ * CHUNK_SIZE * CHUNK_SIZE;

        if (blockX >= 0 && blockX < CHUNK_SIZE && blockY >= 0 && blockY < CHUNK_SIZE && blockZ >= 0 && blockZ < CHUNK_SIZE) {
            if(world[blockID] != 0) {
                if(IsMouseButtonDown(MOUSE_LEFT_BUTTON)) {
                    if(breakingID != blockID) {
                        breakingID = blockID;
                        breakingTime = 0.0f;
                    } else if(breakingTime < 1.0f) {
                        breakingTime += GetFrameTime();
                         model.materials[0].maps[MATERIAL_MAP_DIFFUSE].texture = animations[(int)(breakingTime * 10) % 10];
                        DrawModel(model, (Vector3) {blockX + 0.5f, blockY + 0.5f, blockZ + 0.5f}, 1, WHITE);
                    } else {
                        world[blockID] = 0;
                        breakingID = -1;
                        breakingTime = 0.0f;
                        ReloadMesh();
                    }
                } else if(IsMouseButtonPressed(MOUSE_MIDDLE_BUTTON)) {
                    currentBlock = world[blockID];
                    hotbar[selectedHotbarIndex] = currentBlock;
                } else if(IsMouseButtonPressed(MOUSE_RIGHT_BUTTON) || (IsMouseButtonDown(MOUSE_RIGHT_BUTTON) && IsKeyDown(KEY_LEFT_SHIFT))) {
                    Vector3 normal = DDACursorGetNormal(&cursor);

                    int newBlockX = (int)floor(mapPos.x + normal.x);
                    int newBlockY = (int)floor(mapPos.y + normal.y);
                    int newBlockZ = (int)floor(mapPos.z + normal.z);
                    int newBlockID = newBlockX + newBlockY * CHUNK_SIZE + newBlockZ * CHUNK_SIZE * CHUNK_SIZE;

                    int x = (int)floorf(player.position.x);
                    int y = (int)floorf(player.position.y);
                    int z = (int)floorf(player.position.z);
                    int playerBlockID = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;

                    if (playerBlockID != newBlockID && playerBlockID - CHUNK_SIZE != newBlockID && newBlockX >= 0 && newBlockX < CHUNK_SIZE && newBlockY >= 0 && newBlockY < CHUNK_SIZE && newBlockZ >= 0 && newBlockZ < CHUNK_SIZE) {
                        world[newBlockID] = currentBlock;
                        ReloadMesh();
                    }
                }
                DrawCubeWires((Vector3) {blockX + 0.5f, blockY + 0.5f, blockZ + 0.5f}, 1.02f, 1.02f, 1.02f, WHITE);
                break;
            }
        }
    }

    if(IsMouseButtonUp(MOUSE_LEFT_BUTTON) || i == 8) {
        breakingID = -1;
        breakingTime = 0.0f;
    }
}

void SaveWorld() {
    FILE *file = fopen("world", "wb");
    if (file == NULL) {
        printf("Failed to save world\n");
        return;
    }

    fwrite(world, sizeof(int), CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, file);
    fclose(file);
}

void LoadWorld() {
    FILE *file = fopen("world", "rb");
    if (file == NULL) {
        for (int x = 0; x < CHUNK_SIZE; x++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                for (int y = 0; y < 4; y++) {
                    world[x * CHUNK_SIZE * CHUNK_SIZE + y * CHUNK_SIZE + z] = 2;
                }
                world[x * CHUNK_SIZE * CHUNK_SIZE + 4 * CHUNK_SIZE + z] = 3;
                world[x * CHUNK_SIZE * CHUNK_SIZE + 5 * CHUNK_SIZE + z] = 1;
            }
        }
    }

    fread(world, sizeof(int), CHUNK_SIZE * CHUNK_SIZE * CHUNK_SIZE, file);
    fclose(file);
    printf("World loaded successfully\n");
    ReloadMesh();
}

void ReloadMesh() {
    float vertices[] = {
        0, 0, 1,
        1, 0, 1,
        1, 1, 1,
        0, 1, 1,
        0, 0, 0,
        0, 1, 0,
        1, 1, 0,
        1, 0, 0,
        0, 1, 0,
        0, 1, 1,
        1, 1, 1,
        1, 1, 0,
        0, 0, 0,
        1, 0, 0,
        1, 0, 1,
        0, 0, 1,
        1, 0, 0,
        1, 1, 0,
        1, 1, 1,
        1, 0, 1,
        0, 0, 0,
        0, 0, 1,
        0, 1, 1,
        0, 1, 0
    };

    float texcoords[] = {
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        1.0f, 1.0f,
        0.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 0.0f
    };

    float normals[] = {
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f, 1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 0.0f,-1.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f, 1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        0.0f,-1.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f,
        -1.0f, 0.0f, 0.0f
    };

    Array vert = newArray(32 * 32);
    Array tex = newArray(32 * 32);
    Array norm = newArray(32 * 32);

    for (int x = 0; x < CHUNK_SIZE; x++) {
        for (int y = 0; y < CHUNK_SIZE; y++) {
            for (int z = 0; z < CHUNK_SIZE; z++) {
                int id = x + y * CHUNK_SIZE + z * CHUNK_SIZE * CHUNK_SIZE;
                if(world[id] <= 0) continue;

                for (int i = 0; i < 24; i++) {
                    if(15 < i && i < 20 && x < (CHUNK_SIZE - 1) && world[id + 1] > 0) continue;
                    if(19 < i && i < 24 && x > 0 && world[id - 1] > 0) continue;
                    if(7 < i  && i < 12 && y < (CHUNK_SIZE - 1) && world[id + CHUNK_SIZE] > 0) continue;
                    if(11 < i && i < 16 && y > 0 && world[id - CHUNK_SIZE] > 0) continue;
                    if(i < 4  && z < (CHUNK_SIZE - 1) && world[id + CHUNK_SIZE * CHUNK_SIZE] > 0) continue;
                    if(3 < i  && i < 8  && z > 0 && world[id - CHUNK_SIZE * CHUNK_SIZE] > 0) continue;

                    push(&vert, vertices[i * 3 + 0] + x);
                    push(&vert, vertices[i * 3 + 1] + y);
                    push(&vert, vertices[i * 3 + 2] + z);

                    int type = world[id] - 1;
                    push(&tex, texcoords[i * 2 + 0] * (1.0f / 16) + (type % 16) * (1.0f / 16));
                    push(&tex, texcoords[i * 2 + 1] * (1.0f / 16) + (type / 16) * (1.0f / 16));

                    push(&norm, normals[i * 3 + 0]);
                    push(&norm, normals[i * 3 + 1]);
                    push(&norm, normals[i * 3 + 2]);
                }
            }
        }
    }

    UnloadMesh(worldMesh);

    Mesh mesh = { 0 };
    mesh.vertices = vert.data;
    mesh.texcoords = tex.data;
    mesh.normals = norm.data;
    mesh.vertexCount = vert.size / 3;
    mesh.triangleCount = vert.size / 6;
    mesh.indices = (unsigned short *)RL_MALLOC(vert.size / 2 * sizeof(unsigned short));;

    int k = 0;
    for (int i = 0; i < vert.size / 2; i += 6) {
        mesh.indices[i] = 4*k;
        mesh.indices[i + 1] = 4*k + 1;
        mesh.indices[i + 2] = 4*k + 2;
        mesh.indices[i + 3] = 4*k;
        mesh.indices[i + 4] = 4*k + 2;
        mesh.indices[i + 5] = 4*k + 3;
        k++;
    }

    UploadMesh(&mesh, true);
    worldMesh = mesh;
}