#version 150

in vec3 viewPosition;
in vec3 viewNormal;
in vecc2 paperUV;

out vec4 c;

uniform float contourMin;
uniform float contourMax;
uniform vec3 lightDirection;
uniform sampler2D paperTexture;

uniform float lineWidth;
uniform float omissionThreshold;
uniform float noiseScale;
uniform float frequency;
uniform float phase;
uniform float depthScale;

float calculateBrightness(vec3 N){
    vec3 L = normalize(lightDirection);

    return max(dot(normalize(n),L), 0.0);
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

void main(){
    vec3 N = normalize(ViewNormal);
    vec3 V = normalize(-viewPosition);
    float depth = length(viewPosition);
    float diffuse = calcaulateBrightness(N);
    float thickness = calculateThickness(depth, diffuse);
    float silhouette = calculateSilhouette(N, V, thickness);
    float omission = calculateOmission(paperUV);
    float contour = clamp(silhouette * omission, 0.0, 1.0);

    //testing
    vec3 paperColor = vec3(0.92, 0.89, 0.8);
    vec3 inkColor = vec3(0.04, 0.045, 0.04);

    vec3 finalColor = mix(paperColor, inkColor, contour);
    c = vec4(finalColor, 1.0);
}