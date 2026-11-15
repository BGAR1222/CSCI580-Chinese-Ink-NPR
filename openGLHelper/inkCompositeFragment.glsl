#version 150

in vec3 surfaceNormal;
in vec3 viewPosition;
in vec2 terrainUV;

out vec4 c;

uniform sampler2D noiseTexture;
uniform sampler2D paperTexture;
uniform vec2 viewportSize;
// NEED PAPERTEXTURE in mainTerrain.cpp
uniform float paperStrength;

// for freehand contour
const float depthScale = 0.05;
const float lineWidth = 1.0;
const float contourMin = 0.02;
const float contourMax = 0.10;
const float noiseScale = 1.0;
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
    float paperNoise = texture(paperTexture, uv* noiseScale).r;
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

  // Color Setting
  vec3 basePaperColor = vec3(0.92, 0.89, 0.80);
  vec3 paperColor = basePaperColor;
  vec3 inkColor = vec3(0.04, 0.045, 0.04);
  
  // from inkPaperFragment
  if (paperStrength > 0.0 && viewportSize.x > 0.0 && viewportSize.y > 0.0){
    
    // Convert fragment coordinates into screen-space UV coordinates
    vec2 paperUV = gl_FragCoord.xy / viewportSize;
    
    // Read the grayscale value
    float paperGrain = texture(paperTexture, paperUV).r;
    float paperVariation = (paperGrain - 0.5) * paperStrength;
    paperColor = clamp(basePaperColor * (1.0 + paperVariation),0.0, 1.0);
  }


  float interiorLight = 1.0 - inkValue;
  float contourLight = 1.0 - contourMask;
  //Blend (Multiplication)
  float combinedInk = 1.0 - interiorLight * contourLight;
  // Background Mixing
  c = vec4(mix(paperColor, inkColor, combinedInk), 1.0);
}