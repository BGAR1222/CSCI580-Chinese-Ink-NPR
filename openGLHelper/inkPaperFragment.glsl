#version 150

in vec3 surfaceNormal;
out vec4 c;

uniform sampler2D paperTexture;
uniform vec2 viewportSize;
uniform float paperStrength = 1.0;
uniform float brushStrength = 0.65;
uniform float diffusionStrength = 0.5;
uniform float noiseScale = 1.0;
uniform int previewMode = 0;

float hashNoise(vec2 p)
{
    return fract(sin(dot(p, vec2(127.1, 311.7))) * 43758.5453);
}

float valueNoise(vec2 p)
{
    vec2 cell = floor(p);
    vec2 t = fract(p);
    t = t * t * (3.0 - 2.0 * t);
    return mix(mix(hashNoise(cell), hashNoise(cell + vec2(1, 0)), t.x),
               mix(hashNoise(cell + vec2(0, 1)),
                   hashNoise(cell + vec2(1, 1)), t.x), t.y);
}

float gaussianGrain(vec2 p)
{
    vec2 cell = floor(p);
    float u = max(hashNoise(cell + vec2(19, 73)), 0.0001);
    float v = hashNoise(cell + vec2(53, 11));
    return sqrt(-2.0 * log(u)) * cos(6.2831853 * v);
}

vec3 compositePaperInk(float inputDensity, vec2 paperUV, vec2 pixelPosition)
{
    float density = clamp(inputDensity, 0.0, 1.0);
    float scale = clamp(noiseScale, 0.1, 4.0);
    vec2 p = pixelPosition * scale;
    float grain = texture(paperTexture, clamp(paperUV, 0.0, 1.0)).r;
    vec3 paper = clamp(vec3(0.92, 0.89, 0.80) *
        (1.0 + (grain - 0.5) * clamp(paperStrength, 0.0, 3.0)), 0.0, 1.0);

    // Perturb and soften tone boundaries; no neighbor transport.
    float lowNoise = valueNoise(p * 0.012) - 0.5;
    float wetness = clamp(diffusionStrength, 0.0, 1.0);
    float feather = 0.12 * wetness;
    float wetDensity = smoothstep(-feather, 1.0 + feather,
        density + lowNoise * 0.30 * wetness);
    float edgeWeight = 4.0 * density * (1.0 - density);
    density = mix(density, wetDensity, wetness * edgeWeight);

    // Fine grain and sparse, irregular pores.
    vec2 warped = p + 3.0 * vec2(valueNoise(p * 0.065),
                                valueNoise(p * 0.065 + vec2(37, 91)));
    float pores = valueNoise(warped * 0.7);
    float gaps = 1.0 - smoothstep(0.12, 0.28, pores);
    float grainNoise = clamp(gaussianGrain(p * 0.75), -2.0, 2.0);
    float roughness = clamp(1.0 - 0.08 * max(grainNoise, 0.0), 0.75, 1.0);
    float coverage = roughness * (1.0 - 0.55 * gaps);
    density *= mix(1.0, coverage, clamp(brushStrength, 0.0, 1.0));

    return mix(paper, vec3(0.04, 0.045, 0.04), clamp(density, 0.0, 1.0));
}

void main()
{
    vec2 uv = gl_FragCoord.xy / max(viewportSize, vec2(1.0));
    float density;
    if (previewMode == 1)
    {
        // Analytic swatch for isolated module testing.
        vec2 q = uv - vec2(0.5);
        q.x *= viewportSize.x / max(viewportSize.y, 1.0);
        float stroke = length(q * vec2(0.65, 2.8));
        density = 1.0 - smoothstep(0.30, 0.60, stroke);
    }
    else
    {
        // Replace this test density with the team's interior/contour result.
        vec3 N = normalize(surfaceNormal);
        vec3 L = normalize(vec3(-0.5, 1.0, 0.4));
        density = 1.0 - max(dot(N, L), 0.0);
    }
    c = vec4(compositePaperInk(density, uv, gl_FragCoord.xy), 1.0);
}
