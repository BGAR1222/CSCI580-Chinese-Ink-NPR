#version 150

in vec3 surfaceNormal;
in vec3 viewPosition;
in vec2 terrainUV;

out vec4 c;

uniform sampler2D noiseTexture;
uniform sampler2D paperTexture;
uniform vec2 viewportSize;
// Paper and brush controls.
uniform float paperStrength;
uniform float brushStrength;
uniform float diffusionStrength;
uniform float noiseScale;

uniform int changeView; // 0 = final, 1 = interior ink, 2 = contour, 3 = combined

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



// for freehand contour
const float depthScale = 0.05;
const float lineWidth = 1.0;
const float contourMin = 0.02;
const float contourMax = 0.10;
const float contourNoiseScale = 1.0;
const float frequency = 45.0;
const float phase = 0.0;
const float omissionThreshold = 0.5;

// Li's method for stratify the color of ink black & white (equation 6).
float DiffuseLightLaddering(float dif)
{
  float result = 0.1; //default value if diffuse >= 0.725
  float lerp= 0.0; // t in lerp(a,b,t)
  if (dif <= 0.025){
    result = 1;
  }
  else if (dif <=  0.275){
    lerp= 4*(dif - 0.025);
    result = mix(1.0, 0.7, lerp);
  }
  else if (dif <= 0.400){
    result = 0.7;
  }
  else if (dif <=  0.500){
    lerp= 10*(dif - 0.4);
    result = mix(0.7, 0.3, lerp);
  }
  else if (dif <= 0.625){
    result = 0.3;
  }
  else if (dif <=  0.725){
    lerp= 10*(dif - 0.625);
    result = mix(0.3, 0.1, lerp);
  }

  return result;
}


// Line thickness

float calculateThickness(float depth, float diffuse){
    float depthFactor = clamp(1.0 / (1.0 + depth * depthScale), 0.0, 1.0);

    float brightnessFactor = 1.0 - diffuse;

    float thickness = lineWidth * mix(0.5,1.5,depthFactor) * mix(0.6,1.4,brightnessFactor);

    return thickness;
}

float calculateSilhouette(vec3 N, vec3 V, float thickness){
    float facing = abs(dot(normalize(N), normalize(V)));

    float baseWidth = contourMax - contourMin;

    float adjustMax = clamp(contourMin + baseWidth * thickness, contourMin + 0.00001, 1.0);
    return 1.0 - smoothstep(contourMin, adjustMax, facing);
}

float calculateOmission(vec2 uv){
    float paperNoise = texture(paperTexture, uv * contourNoiseScale).r;
    float periodic = sin(uv.x * frequency + phase) * 0.2;
    float pattern = paperNoise + periodic;
    return step(omissionThreshold, pattern);
}

float freehandContour(vec3 normal, vec3 viewDir, float diffuse)
{
    float depth = length(viewPosition);
    float thickness = calculateThickness(depth, diffuse);
    float silhouette = calculateSilhouette(normal, viewDir, thickness);
    float omission = calculateOmission(terrainUV);
    float contour = clamp(silhouette * omission, 0.0, 1.0);

  return contour;
}

void main()
{
  vec3 normal = normalize(surfaceNormal);
  vec3 viewDir = normalize(-viewPosition);
  vec3 lightDir = normalize(vec3(-0.5, 1.0, 0.4));

  // Interior ink
  float diffuse = max(dot(normal, lightDir), 0.0);
  float inkValue = DiffuseLightLaddering(diffuse);

  // Freehand contour, from 0 (no line) to 1 (solid ink)
  float contourMask = freehandContour(normal, viewDir, diffuse);

  // Combine interior and contour ink before paper compositing.
  float interiorLight = 1.0 - inkValue;
  float contourLight = 1.0 - contourMask;
  float combinedInk = 1.0 - interiorLight * contourLight;

  // Change views in window
  if (changeView == 1){ 
    // only show Diffuse Light Laddering 
    c = vec4(vec3(interiorLight), 1.0);
    return;
  }

  if (changeView == 2){
    // only show Contour Line
    c = vec4(vec3(contourLight), 1.0);
    return;
  }

  if (changeView == 3){
    // combined interior ink and contours
    c = vec4(vec3(interiorLight * contourLight), 1.0);
    return;
  }
  // final view with Background Mixing and Noise Texture Mapping
  vec2 paperUV = gl_FragCoord.xy / max(viewportSize, vec2(1.0));
  c = vec4(compositePaperInk(combinedInk, paperUV, gl_FragCoord.xy), 1.0);
}
