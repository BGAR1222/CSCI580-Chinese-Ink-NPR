#version 150

in vec3 surfaceNormal;
in vec3 viewPosition;

out vec4 gBuffer;

void main()
{
    // Interpolated normals are not guaranteed to remain unit length.
    vec3 N = normalize(surfaceNormal);

    // Convert the normal from [-1, 1] to [0, 1]
    // so it can be directly visualized as RGB.
    vec3 encodedNormal =
        N * 0.5 + 0.5;

    // A point in front of the camera normally has negative
    // view-space Z. Negating it gives positive linear depth.
    //
    // Zero is reserved for the background, so use a small
    // positive lower bound for model pixels.
    float linearDepth =
        max(-viewPosition.z, 0.0001);

    // RGB: encoded view-space normal
    // A:   linear view-space depth
    gBuffer = vec4(
        encodedNormal,
        linearDepth
    );
}