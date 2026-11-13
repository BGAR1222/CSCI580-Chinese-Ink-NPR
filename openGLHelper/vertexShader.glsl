#version 150
//you may desire additional data like the size of the mesh to perform texture lookups
//you have 2 options, 

//1. you can pass uv values per vertex in an additional VBO+VAO
in vec3 position;

out float height;
out vec3 surfaceNormal;
out vec3 viewPosition;

//2. *or* you can compute what would be the uv's on the fly by passing in the dimensions of the mesh as uniforms
uniform mat4 modelViewMatrix;
uniform mat4 projectionMatrix;
uniform sampler2D noiseTexture;
uniform float terrainSize;
uniform float amplitude;

/*
	The Vertex shader is responsible for any manipulations to the vertices themselves.
	You may also compute per-vertex data and out it to the fragment shader(make sure the variable names and types match). 
	In this case you are computing the height of the terrain and passing it to the fragment shader for coloring.
	The Graphics pipeline will automatically reconstruct the Triangles, rasterize, and interpolate the vertex data onto the fragments.
*/

void main()
{
  //this performs the traditional 3D to 2D projection.
  //modelViewMatrix transforms the vertex from model space to view space
  //projectionMatrix performs perspective shearing.
  //in lecture we did this with 4 matricies but it is more common in code to combine them into one matrix and do the multiplication in one step.

  
  vec2 uv = vec2(
    position.x / terrainSize + 0.5,
    position.z / terrainSize+ 0.5
  );

  height = texture( noiseTexture , uv).r;

  float heightLeft = textureOffset(
    noiseTexture, uv, ivec2(-1, 0)
  ).r;

  float heightRight = textureOffset(
    noiseTexture, uv, ivec2(1, 0)
  ).r;

  float heightBack = textureOffset(
    noiseTexture, uv, ivec2(0, -1)
  ).r;

  float heightFront = textureOffset(
    noiseTexture, uv, ivec2(0, 1)
  ).r;

  float heightDifferenceX = heightRight - heightLeft;
  float heightDifferenceZ = heightFront - heightBack;

  ivec2 textureDimensions = textureSize(noiseTexture, 0);

  float oneTexelWorldDistance = terrainSize / float(textureDimensions.x);

  // Left-to-right and back-to-front both span two texels
  float sampleWorldSpan = oneTexelWorldDistance * 2.0;

  // apply the same height scaling used for vertex displacement
  float displacementScale = amplitude * 2.0; 

  vec3 tangentX = vec3(sampleWorldSpan, heightDifferenceX * displacementScale, 0.0);
  vec3 tangentZ = vec3(0.0, heightDifferenceZ * displacementScale, sampleWorldSpan);




  float centeredHeight = height * 2 - 1;

  vec3 displacedPosition = position;
  displacedPosition.y = position.y + centeredHeight * amplitude;

  vec4 positionInViewSpace =
    modelViewMatrix * vec4(displacedPosition, 1.0);

  viewPosition = positionInViewSpace.xyz;

  gl_Position = projectionMatrix * positionInViewSpace;

  // Derive the terrain normal from the height texture and transform it to view space
  vec3 modelNormal = normalize(cross(tangentZ, tangentX));

  mat3 normalMatrix = transpose(inverse(mat3(modelViewMatrix)));

  surfaceNormal = normalize(normalMatrix * modelNormal);
}