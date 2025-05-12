#version 330

// Input vertex attributes (from vertex shader)
in vec2 fragTexCoord;
in vec3 fragPosition;
in vec3 fragNormal;

// Input uniform values
uniform sampler2D texture0;
uniform vec3 viewPos;

// Output fragment color
out vec4 finalColor;

void main()
{
    // Texture sampling
    vec4 texelColor = texture(texture0, fragTexCoord);

    // Ambient lighting
    float ambientStrength = 0.7;
    vec3 ambient = ambientStrength * vec3(1.0, 1.0, 1.0);

    // Diffuse lighting
    vec3 lightDir = normalize(vec3(0.3, 1.0, 0.7)); // Static light direction
    float diff = abs(dot(fragNormal, lightDir));
    vec3 diffuse = diff * vec3(0.5);

    // Combine results
    vec3 result = (ambient + diffuse) * texelColor.rgb;

    // Apply fog
    float fogStart = 10.0;
    float fogEnd = 30.0;
    vec3 fogColor = vec3(0.6, 0.8, 1.0); // Light blue sky color

    float dist = length(fragPosition - viewPos);
    float fogFactor = clamp((fogEnd - dist) / (fogEnd - fogStart), 0.0, 1.0);

    result = mix(fogColor, result, fogFactor);

    finalColor = vec4(result, texelColor.a);
}