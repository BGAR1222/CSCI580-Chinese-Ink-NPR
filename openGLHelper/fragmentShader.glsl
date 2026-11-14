#version 150

in float height;
in vec3 surfaceNormal;
in vec3 viewPosition;

out vec4 c;

// Interior rendering

// Li's method for stratify the color of ink black & white (equation 6).
float DiffuseLightLaddering(float dif)
{
  float result = 0.1; //default value if diffuse >= 0.725
  float lerp= 0.0; // t in lerp(a,b,t)
  if (dif <= 0.025){
    result = 1;
  }
  if (dif <=  0.275){
    float lerp= 4*(dif - 0.025);
    result = mix(1.0, 0.7, lerp);
  }
  if (dif <= 0.400){
    result = 0.7;
  }
  if (dif <=  0.500){
    float lerp= 10*(dif - 0.4);
    result = mix(0.7, 0.3, lerp);
  }
  if (dif <= 0.625){
    result = 0.3;
  }
  if (dif <=  0.725){
    float lerp= 10*(dif - 0.625);
    result = mix(0.3, 0.1, lerp);
  }

  return result;
}

/*
	The fragment shader is the last step before z-testing,blending, and output to the frame buffer
	Similar to shadertoy we want a vec4 color as our output.
	We only need the terrain height to color the terrain, but you can add additional inputs if you like
*/
void main()
{

  // From Li, use Lambert diffused light definition
  // diffuse = max(dot(world_light_direction, world_normal),0)

  //fixed viewspace direction from the terrain surface toward the light
  vec3 lightDirection = normalize(vec3(-0.5, 1.0, 0.4));

  // Re-normalize the interpolated surface normal before lighting
  vec3 normal = normalize(surfaceNormal);

  // NEW: ink diffusion mimic
  float diffuse = max(dot(lightDirection, normal), 0.0);
  float inkDiffusion = DiffuseLightLaddering(diffuse);

  // output
  c = vec4(vec3(inkDiffusion), 1.0);
}

