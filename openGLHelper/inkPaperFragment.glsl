#version 150

in vec3 surfaceNormal;

out vec4 c;

uniform sampler2D paperTexture;
uniform vec2 viewportSize;
uniform float paperStrength;

void main()
{
    // Temp ink density from diffuse lighting
    vec3 N = normalize(surfaceNormal);
    vec3 L = normalize(vec3(-0.5, 1.0, 0.4));

    float diffuse = max(dot(N, L), 0.0);
    float inkDensity = 1.0 - diffuse;

    // Convert fragment coordinates into screen-space UV coordinates
    vec2 paperUV = gl_FragCoord.xy / viewportSize;

    // Read the grayscale value
    float paperGrain = texture(paperTexture, paperUV).r;

    float paperVariation =
        (paperGrain - 0.5) * paperStrength;

    vec3 basePaperColor = vec3(0.92, 0.89, 0.80);
    vec3 inkColor = vec3(0.04, 0.045, 0.04);

    // Modulate the paper color
    vec3 paper = clamp(
        basePaperColor * (1.0 + paperVariation),
        0.0,
        1.0
    );

    // Blend 
    vec3 result = mix(paper, inkColor, inkDensity);

    c = vec4(result, 1.0);
}