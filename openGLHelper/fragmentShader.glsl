#version 150

in float height;
in vec3 surfaceNormal;
in vec3 viewPosition;

out vec4 c;

/*
	The fragment shader is the last step before z-testing,blending, and output to the frame buffer
	Similar to shadertoy we want a vec4 color as our output.
	We only need the terrain height to color the terrain, but you can add additional inputs if you like
*/

void main()
{
  vec3 lowestColor = vec3(168,49,247) / 255.0;
  vec3 lowColor = vec3(247,49,148) / 255.0;

  vec3 highColor = vec3(247,109,49) / 255.0;
  vec3 highestColor = vec3(247,228,49) / 255.0;

 //softer and broader transition
  vec3 terrainColor = lowestColor;

  terrainColor = mix(
    terrainColor,
    lowColor,
    smoothstep(0.35, 0.45, height)
  );

  terrainColor = mix(
    terrainColor,
    highColor,
    smoothstep(0.45, 0.54, height)
  );

  terrainColor = mix(
    terrainColor,
    highestColor,
    smoothstep(0.55, 0.68, height)
  );



  //fixed viewspace direction from the terrain surface toward the light
  vec3 lightDirection = normalize(vec3(-0.5, 1.0, 0.4));

  // Re-normalize the interpolated surface normal before lighting
  vec3 normal = normalize(surfaceNormal);

  vec3 viewDirection = normalize(-viewPosition);

  vec3 reflectionDirection = reflect(-lightDirection, normal);

  float shininess = 32.0;

  float specularStrength = pow(max(dot(viewDirection, reflectionDirection), 0.0), shininess);

  float diffuseStrength = max(dot(normal,lightDirection), 0.0);

  float ambientStrength = 0.2;

  float lightingStrength = ambientStrength + diffuseStrength * 0.8;

  vec3 litTerrainColor = terrainColor * lightingStrength;

  float specularAmount = 0.35;

  vec3 specularColor = vec3(1.0) * specularStrength * specularAmount;

  c = vec4(litTerrainColor + specularColor, 1.0);

}

