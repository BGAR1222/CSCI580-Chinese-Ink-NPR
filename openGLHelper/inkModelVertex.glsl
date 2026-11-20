#version 150

in vec3 position;
in vec3 normal;
in vec2 modelUV;

out vec3 surfaceNormal;
out vec3 viewPosition;
out vec2 terrainUV;

uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;

void main()
{
    vec4 viewVertex = modelViewMatrix * vec4(position, 1.0);
    viewPosition = viewVertex.xyz;
    surfaceNormal = normalize(transpose(inverse(mat3(modelViewMatrix))) * normal);
    terrainUV = modelUV;
    gl_Position = projectionMatrix * viewVertex;
}
