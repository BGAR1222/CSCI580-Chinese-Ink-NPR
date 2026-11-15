#version 150

in float height;
in vec3 surfaceNormal;
in vec3 viewPosition;
in vec3 viewDirection;
in float viewDepth;
in vec2 terrainUV;

out vec4 c;

uniform int debugMode;

void main()
{
    vec3 N = normalize(surfaceNormal);
    vec3 V = normalize(viewDirection);

    float facing = abs(dot(N, V));

    const float contourStart = 0.05;
    const float contourEnd = 0.25;

    float contour =
        1.0 - smoothstep(contourStart, contourEnd, facing);

    vec3 paperColor = vec3(0.93, 0.90, 0.82);
    vec3 inkColor = vec3(0.03, 0.035, 0.03);

    if (debugMode == 1)
    {
        // Normal visualization
        c = vec4(N * 0.5 + 0.5, 1.0);
    }
    else if (debugMode == 2)
    {
        // N dot V visualization
        c = vec4(vec3(facing), 1.0);
    }
    else if (debugMode == 3)
    {
        // Depth visualization
        float depthDisplay =
            1.0 - clamp(viewDepth / 30.0, 0.0, 1.0);

        c = vec4(vec3(depthDisplay), 1.0);
    }
    else
    {
        vec3 finalColor =
            mix(paperColor, inkColor, contour);

        c = vec4(finalColor, 1.0);
    }
}