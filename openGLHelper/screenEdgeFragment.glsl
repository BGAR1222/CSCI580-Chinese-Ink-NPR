#version 150 

in vec2 screenUV;

uniform sampler2D gBufferTexture;
uniform vec2 texelSize;

uniform int debugView;
uniform float depthThreshold;
uniform float normalThreshold;
uniform float depthDisplayScale;

out vec4 fragColor;

void main(){
    vec4 center = texture(gBufferTexture, screenUV);
    vec3 centerNormal = normalize(center.rgb * 2.0 - 1.0);
    float centerDepth = center.a;
    float centerMask = step(0.0001, centerDepth);

    if (debugView == 1){
        fragColor = vec4(center.rgb, 1.0);
        return;
    }

    if (debugView == 2){
        float d = clamp(centerDepth / max(depthDisplayScale, 0.0001), 0.0, 1.0);
        fragColor = vec4(vec3(d),1.0);
        return;
    }

        if (debugView == 3){
        fragColor = vec4(vec3(centerMask),1.0);
        return;
    }

    vec2 offsets[4] = vec2[4](vec2( 1.0,  0.0), vec2(-1.0,  0.0), vec2( 0.0,  1.0), vec2( 0.0, -1.0));

    float maskEdge = 0.0;
    float depthEdge = 0.0;
    float normalEdge = 0.0;

    for(int i  = 0; i < 4, i++){
        vec4 neighbor = texture(gBufferTexture, screenUV + offsets[i] * texelSize);
        float neighborDepth = neighbor.a;
        float neighborMask = step(0.0001, neighborDepth);

        maskEdge = max(maskEdge, abs(centerMask - neighborMask));

        // Detect depth discontinuities
        if(centerMask > 0.5 && neighborMask > 0.5){
            float depthDifference = abs(centerDepth - neighborDepth) / max(min(centerDepth,neighborDepth), 0.001);

            depthEdge = max(depthEdge, smoothstep(depthThreshold, depthThreshold * 2.0, depthDifference));
            
            vec3 neighborNormal = normalize(neighbor.rbg * 2.0 - 1.0);

            float normalDifference = 1.0 - clamp(dot(centerNormal, neighborNormal), -1.0, 1.0);
            normalEdge = max(normalEdge, smoothstep(normalThreshold, normalThreshold * 2.0, normalDifference));
        }
    }

    float outerEdge = max(maskEdge, depthEdge);
    float innerEdge = normalEdge * (1.0 - outerEdge);
    float combinedEdge = max(outerEdge, innerEdge * 0.45);

    if (debugView == 4)
    {
        fragColor = vec4(vec3(1.0 - maskEdge), 1.0);
        return;
    }

    if (debugView == 5)
    {
        fragColor = vec4(vec3(1.0 - depthEdge), 1.0);
        return;
    }

    if (debugView == 6)
    {
        fragColor = vec4(vec3(1.0 - normalEdge), 1.0);
        return;
    }

    fragColor = vec4(vec3(1.0 - combinedEdge), 1.0);
}