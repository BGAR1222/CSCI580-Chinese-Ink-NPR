#version 150

in vec3 surfaceNormal;
in vec3 viewPosition;
in vec2 terrainUV;

out vec4 c;

uniform sampler2D noiseTexture;
uniform sampler2D paperTexture;
uniform vec2 viewportSize;
uniform float paperStrength;

// Li's method for stratify the color of ink black & white (equation 6).
float DiffuseLightLaddering(float dif)
{
  float result = 0.1; //default value if diffuse >= 0.725
  float lerp= 0.0; // t in lerp(a,b,t)
  if (dif <= 0.025){
    result = 1;
  }
  else if (dif <=  0.275){
    float lerp= 4*(dif - 0.025);
    result = mix(1.0, 0.7, lerp);
  }
  else if (dif <= 0.400){
    result = 0.7;
  }
  else if (dif <=  0.500){
    float lerp= 10*(dif - 0.4);
    result = mix(0.7, 0.3, lerp);
  }
  else if (dif <= 0.625){
    result = 0.3;
  }
  else if (dif <=  0.725){
    float lerp= 10*(dif - 0.625);
    result = mix(0.3, 0.1, lerp);
  }

  return result;
}

float freehandContour(vec3 normal, vec3 viewDir, float diffuse)
{
  // Near-grazing surfaces are candidates for contour ink.
  float facing = abs(dot(normal, viewDir));

  // Wider in darker areas and narrower farther from the camera.
  float depth = length(viewPosition);
  float depthFactor = 1.0 / (1.0 + 0.05 * depth);
  float width = 0.08 * mix(0.5, 1.5, depthFactor)
                     * mix(0.6, 1.4, 1.0 - diffuse);

  float silhouette = 1.0 - smoothstep(0.02, 0.02 + width, facing);

  // Reuse the terrain noise texture to break up the line.
  float noise = texture(noiseTexture, terrainUV).r;
  float wave = 0.2 * sin(terrainUV.x * 45.0);
  float keepLine = step(0.35, noise + wave);

  return silhouette * keepLine;
}

void main()
{
  vec3 normal = normalize(surfaceNormal);
  vec3 viewDir = normalize(-viewPosition);
  vec3 lightDir = normalize(vec3(-0.5, 1.0, 0.4));

  // Interior ink.
  float diffuse = max(dot(normal, lightDir), 0.0);
  float inkValue = DiffuseLightLaddering(diffuse);

  // Freehand contour, from 0 (no line) to 1 (solid ink).
  float contourMask = freehandContour(normal, viewDir, diffuse);

  // Paper color, with optional screen-space grain.
  vec3 paperColor = vec3(0.92, 0.89, 0.80);
  vec3 inkColor = vec3(0.04, 0.045, 0.04);

  if (paperStrength > 0.0 && viewportSize.x > 0.0 && viewportSize.y > 0.0)
  {
    vec2 paperUV = gl_FragCoord.xy / viewportSize;
    float grain = texture(paperTexture, paperUV).r;
    paperColor = clamp(paperColor * (1.0 + (grain - 0.5) * paperStrength),
                       0.0, 1.0);
  }

  // Multiply the light remaining after interior and contour ink.
  float interiorLight = 1.0 - inkValue;
  float contourLight = 1.0 - contourMask;
  float combinedInk = 1.0 - interiorLight * contourLight;

  c = vec4(mix(paperColor, inkColor, combinedInk), 1.0);
}