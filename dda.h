typedef struct DDACursor {
    Vector3 mask;
    Vector3 mapPos;
    Vector3 rayDir;
    Vector3 deltaDist;
    Vector3 sideDist;
    Vector3 step;
} DDACursor;

DDACursor DDACursorCreate(Vector3 pos, Vector3 rayDir) {
    DDACursor cursor;

    cursor.mapPos = (Vector3){ floor(pos.x), floor(pos.y), floor(pos.z) };
    cursor.rayDir = rayDir;

    cursor.deltaDist = (Vector3){
        (rayDir.x == 0) ? INFINITY : fabs(1.0f / rayDir.x),
        (rayDir.y == 0) ? INFINITY : fabs(1.0f / rayDir.y),
        (rayDir.z == 0) ? INFINITY : fabs(1.0f / rayDir.z)
    };

    cursor.step = (Vector3){
        (rayDir.x < 0) ? -1.0f : 1.0f,
        (rayDir.y < 0) ? -1.0f : 1.0f,
        (rayDir.z < 0) ? -1.0f : 1.0f
    };

    cursor.sideDist = (Vector3){
        (rayDir.x < 0) ? (pos.x - cursor.mapPos.x) * cursor.deltaDist.x : (cursor.mapPos.x + 1.0f - pos.x) * cursor.deltaDist.x,
        (rayDir.y < 0) ? (pos.y - cursor.mapPos.y) * cursor.deltaDist.y : (cursor.mapPos.y + 1.0f - pos.y) * cursor.deltaDist.y,
        (rayDir.z < 0) ? (pos.z - cursor.mapPos.z) * cursor.deltaDist.z : (cursor.mapPos.z + 1.0f - pos.z) * cursor.deltaDist.z
    };

    cursor.mask = (Vector3){ 0, 0, 0 };

    return cursor;
}

Vector3 DDACursorStep(DDACursor *cursor) {
    if (cursor->sideDist.x < cursor->sideDist.y && cursor->sideDist.x < cursor->sideDist.z) {
        cursor->sideDist.x += cursor->deltaDist.x;
        cursor->mapPos.x += cursor->step.x;
        cursor->mask = (Vector3){ 1, 0, 0 };
    } else if (cursor->sideDist.y < cursor->sideDist.z) {
        cursor->sideDist.y += cursor->deltaDist.y;
        cursor->mapPos.y += cursor->step.y;
        cursor->mask = (Vector3){ 0, 1, 0 };
    } else {
        cursor->sideDist.z += cursor->deltaDist.z;
        cursor->mapPos.z += cursor->step.z;
        cursor->mask = (Vector3){ 0, 0, 1 };
    }

    return cursor->mapPos;
}

Vector3 DDACursorGetNormal(DDACursor *cursor) {
    Vector3 normal = {0};

    if (cursor->mask.x != 0) {
        normal.x = -cursor->step.x;
    } else if (cursor->mask.y != 0) {
        normal.y = -cursor->step.y;
    } else if (cursor->mask.z != 0) {
        normal.z = -cursor->step.z;
    }

    return normal;
}