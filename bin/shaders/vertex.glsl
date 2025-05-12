#version 330

// Input vertex attributes
in vec3 vertexPosition;
in vec2 vertexTexCoord;
in vec3 vertexNormal;

// Input uniform values
uniform mat4 mvp;
uniform mat4 matModel;

// Output vertex attributes (to fragment shader)
out vec2 fragTexCoord;
out vec3 fragPosition;
out vec3 fragNormal;

void main()
{
    // Calculate fragment position in world space
    fragPosition = vec3(matModel * vec4(vertexPosition, 1.0));

    // Calculate fragment normal in world space
    fragNormal = normalize(mat3(matModel) * vertexNormal);

    // Send vertex attributes to fragment shader
    fragTexCoord = vertexTexCoord;

    // Calculate final vertex position
    gl_Position = mvp * vec4(vertexPosition, 1.0);
}
