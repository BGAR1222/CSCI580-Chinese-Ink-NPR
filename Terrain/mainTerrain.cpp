/*

  CSCI 580 Computer Graphics, Computer Science, USC
  Procedural Generation Assignment Starter Code
  Carter Slocum, PhD. + origional pipeline code by Jernej Barbic, PhD.
  Student username: jiaruipe

  Read the comments!
  The primary functions that you need to work in are:
  - generateGradientGrid(): called to initialize the random 2d unit vectors.
  - perlinNoise(): called to interpolate a single noise value for a given x,y coordinate.
  - createPerlinTexture(): calls perlinNoise() repeatedly to fill a 2D array of floats with noise values.
  - initScene(): called once at the beginning of the program, to initialize the scene
	you need to implement the procedural terrain mesh generation and call the noise texture generation in this function.
  - displayFunc(): called every time the scene needs to be rendered.
    If any per-frame information needs to be updated (passing camera position to the shader), here is the best place.
*/

#include "openGLHeader.h"
#include "glutHeader.h"
#include "openGLMatrix.h"
#include "imageIO.h"
#include "pipelineProgram.h"
#include "vbo.h"
#include "vao.h"
#include "Texture.h"

#include <iostream>
#include <cstring>
#include <cstdlib>
#include <ctime>
#include <cmath>
#include <glm/glm.hpp>

#if defined(WIN32) || defined(_WIN32)
  #ifdef _DEBUG
    #pragma comment(lib, "glew32d.lib")
  #else
    #pragma comment(lib, "glew32.lib")
  #endif
#endif

#if defined(WIN32) || defined(_WIN32)
  char shaderBasePath[1024] = SHADER_BASE_PATH;
#else
  char shaderBasePath[1024] = "../openGLHelper";
#endif

using namespace std;

int mousePos[2]; // x,y screen coordinates of the current mouse position

int leftMouseButton = 0; // 1 if pressed, 0 if not 
int middleMouseButton = 0; // 1 if pressed, 0 if not
int rightMouseButton = 0; // 1 if pressed, 0 if not

typedef enum { ROTATE, TRANSLATE, SCALE } CONTROL_STATE;
CONTROL_STATE controlState = ROTATE;

// Transformations of the terrain.
float terrainRotate[3] = { 0.0f, 0.0f, 0.0f }; 
// terrainRotate[0] gives the rotation around x-axis (in degrees)
// terrainRotate[1] gives the rotation around y-axis (in degrees)
// terrainRotate[2] gives the rotation around z-axis (in degrees)
float terrainTranslate[3] = { 0.0f, 0.0f, 0.0f };
float terrainScale[3] = { 1.0f, 1.0f, 1.0f };

// Width and height of the OpenGL window, in pixels.
int windowWidth = 1280;
int windowHeight = 720;
char windowTitle[512] = "CSCI580 Chinese Ink NPR";

int numVertices;

// Graphics helper classes.
OpenGLMatrix matrix;
PipelineProgram * pipelineProgram = nullptr;
VBO * vboVertices = nullptr;
VBO * vboColors = nullptr;
VAO * vao = nullptr;
Texture * noiseTexture = nullptr;

//feel free to mess with the gradient grid size
const int gradientGridSize = 50;
//this is a 2D array of unit vectors that will be used to generate the noise texture.
glm::vec2 gradientGrid[gradientGridSize][gradientGridSize];

void generateGradientGrid()
{
    for(int y = 0; y < gradientGridSize; ++y)
    {
      for(int x = 0; x < gradientGridSize; ++x)
      {
        // Map a random value to an angle covering a full circle.
        // Cosine and sine produce a unit vector pointing in that direction.
        const float random01 = static_cast<float>(rand()) / static_cast<float>(RAND_MAX);

        const float angle = random01 * 6.2831853f;

        gradientGrid[y][x] = glm::vec2(
          std::cos(angle),
          std::sin(angle)
        );
      }
    }
    
}


//Function should return a value between -1 and 1,
//Assumes that the gradient grid has been generated and is available as a global variable.
float perlinNoise(float x, float y)
{

  //const float gridX = x * (gradientGridSize -1);
  //const float gridY = y * (gradientGridSize - 1);

  const float gridX = x ;
  const float gridY = y ;

  const int x0 = static_cast<int>(std::floor(gridX));
  const int y0 = static_cast<int>(std::floor(gridY));

  const int x1 = x0 + 1;
  const int y1 = y0 + 1;

  // Fractional position of the sample within its current gradient cell.
  const float localX = gridX - static_cast<float>(x0);
  const float localY = gridY - static_cast<float>(y0);

  // Displacements from each cell corner to the sample point.
  const glm::vec2 d00(localX, localY);
  const glm::vec2 d10(localX - 1.0f, localY);
  const glm::vec2 d01(localX, localY - 1.0f);
  const glm::vec2 d11(localX - 1.0f, localY - 1.0f);

  // Dot products measure how each corner gradient aligns with its displacement.
  const float dot00 = glm::dot(gradientGrid[y0][x0], d00);
  const float dot10 = glm::dot(gradientGrid[y0][x1], d10);
  const float dot01 = glm::dot(gradientGrid[y1][x0], d01);
  const float dot11 = glm::dot(gradientGrid[y1][x1], d11);

  //smooth the interpolation weights to avoid visible cell boundaries
  const float smoothX = localX * localX * (3.0f - 2.0f * localX);
  const float smoothY = localY * localY * (3.0f - 2.0f * localY);

  //blend the four corner contributions using the smoothed cell position
  const float bottom = dot00 * (1.0f - smoothX) + dot10 * smoothX;
  const float top = dot01 * (1.0f - smoothX) + dot11 * smoothX;
  const float noiseValue = bottom * (1.0f - smoothY) + top * smoothY;

  return noiseValue;
}

// This function allocates and fills a 2D array of floats.
// as is, this is enough to get started,
// but you will want to add fractal lookups to this function to get a more interesting texture.

float ** createPerlinTexture(int size)
{
  float ** pixels = static_cast<float **>(malloc(size * sizeof(float *)));
  if (pixels == nullptr)
    return nullptr;

  for (int y = 0; y < size; ++y)
  {
    pixels[y] = static_cast<float *>(malloc(size * sizeof(float)));
    if (pixels[y] == nullptr)
    {
      for (int allocatedRow = 0; allocatedRow < y; ++allocatedRow)
        free(pixels[allocatedRow]);
      free(pixels);
      return nullptr;
    }
  }


  float minTextureValue  = 1.0f;
  float maxTextureValue = -1.0f; 

  // Span several gradient cells so the base noise has broad, visible features.
  // Later, fractal noise will sample additional octaves at higher frequencies.
  const float baseFrequency = 4.0f;
  const int octaveCount = 4;
  const float lacunarity = 2.0f;
  const float persistence = 0.5f;

  // Combine four octaves: higher frequencies add smaller details,
  // while persistence reduces their influence.
  for (int y = 0; y < size; ++y)
  {
    for (int x = 0; x < size; ++x)
	{
		const float sampleX = static_cast<float>(x) / size;
		const float sampleY = static_cast<float>(y) / size;

    float noiseSum = 0.0f;
    float weightSum = 0.0f;

    float frequency = baseFrequency;
    float octaveWeight = 1.0f;

    for(int octave = 0; octave < octaveCount; ++octave)
    {
      const float octaveNoise = perlinNoise(
        sampleX * frequency,
        sampleY * frequency
      );

      noiseSum += octaveNoise * octaveWeight;
      weightSum += octaveWeight;

      frequency *= lacunarity;
      octaveWeight *=persistence;
    }

    const float rawNoise = noiseSum / weightSum;
		
    // Remap raw Perlin noise from [-1, 1] to [0, 1] for texture storage
    pixels[y][x] = (rawNoise + 1.0f) / 2.0f ;
    
    if(pixels[y][x] < minTextureValue)
    {
      minTextureValue = pixels[y][x];
    }

    if(pixels[y][x] > maxTextureValue)
    {
      maxTextureValue =  pixels[y][x];
    }
    }
  }

  cout << "Perlin noise range: "
     << minTextureValue << " to " << maxTextureValue << endl;

  return pixels;
}
/* The Following are funcitons for GLUT to handle events like window resizing and mouse input 
  You can leave these as they are*/

// Write a screenshot to the specified filename.
void saveScreenshot(const char * filename)
{
  unsigned char * screenshotData = new unsigned char[windowWidth * windowHeight * 3];
  glReadPixels(0, 0, windowWidth, windowHeight, GL_RGB, GL_UNSIGNED_BYTE, screenshotData);

  ImageIO screenshotImg(windowWidth, windowHeight, 3, screenshotData);

  if (screenshotImg.save(filename, ImageIO::FORMAT_JPEG) == ImageIO::OK)
    cout << "File " << filename << " saved successfully." << endl;
  else cout << "Failed to save file " << filename << '.' << endl;

  delete [] screenshotData;
}

// GLUT idle callback function.
void idleFunc()
{
  // Do some stuff... 
  // For example, here, you can save the screenshots to disk (to make the animation).
  // Notify GLUT that it should call displayFunc.
  glutPostRedisplay();
}

//handle window resizing
void reshapeFunc(int w, int h)
{
  glViewport(0, 0, w, h);

  // When the window has been resized, we need to re-set our projection matrix.
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.LoadIdentity();
  // You need to be careful about setting the zNear and zFar. 
  // Anything closer than zNear, or further than zFar, will be culled.
  const float zNear = 0.1f;
  const float zFar = 10000.0f;
  const float humanFieldOfView = 60.0f;
  matrix.Perspective(humanFieldOfView, 1.0f * w / h, zNear, zFar);
}

void mouseMotionDragFunc(int x, int y)
{
  // Mouse has moved, and one of the mouse buttons is pressed (dragging).

  // the change in mouse position since the last invocation of this function
  int mousePosDelta[2] = { x - mousePos[0], y - mousePos[1] };

  switch (controlState)
  {
    // translate the terrain
    case TRANSLATE:
      if (leftMouseButton)
      {
        // control x,y translation via the left mouse button
        terrainTranslate[0] += mousePosDelta[0] * 0.01f;
        terrainTranslate[1] -= mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z translation via the middle mouse button
        terrainTranslate[2] += mousePosDelta[1] * 0.01f;
      }
      break;

    // rotate the terrain
    case ROTATE:
      if (leftMouseButton)
      {
        // control x,y rotation via the left mouse button
        terrainRotate[0] += mousePosDelta[1];
        terrainRotate[1] += mousePosDelta[0];
      }
      if (middleMouseButton)
      {
        // control z rotation via the middle mouse button
        terrainRotate[2] += mousePosDelta[1];
      }
      break;

    // scale the terrain
    case SCALE:
      if (leftMouseButton)
      {
        // control x,y scaling via the left mouse button
        terrainScale[0] *= 1.0f + mousePosDelta[0] * 0.01f;
        terrainScale[1] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      if (middleMouseButton)
      {
        // control z scaling via the middle mouse button
        terrainScale[2] *= 1.0f - mousePosDelta[1] * 0.01f;
      }
      break;
  }

  // store the new mouse position
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseMotionFunc(int x, int y)
{
  // Mouse has moved.
  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void mouseButtonFunc(int button, int state, int x, int y)
{
  // A mouse button has has been pressed or depressed.

  // Keep track of the mouse button state, in leftMouseButton, middleMouseButton, rightMouseButton variables.
  switch (button)
  {
    case GLUT_LEFT_BUTTON:
      leftMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_MIDDLE_BUTTON:
      middleMouseButton = (state == GLUT_DOWN);
    break;

    case GLUT_RIGHT_BUTTON:
      rightMouseButton = (state == GLUT_DOWN);
    break;
  }

  // Keep track of whether CTRL and SHIFT keys are pressed.
  switch (glutGetModifiers())
  {
    case GLUT_ACTIVE_CTRL:
      controlState = TRANSLATE;
    break;

    case GLUT_ACTIVE_SHIFT:
      controlState = SCALE;
    break;

    // If CTRL and SHIFT are not pressed, we are in rotate mode.
    default:
      controlState = ROTATE;
    break;
  }

  // Store the new mouse position.
  mousePos[0] = x;
  mousePos[1] = y;
}

void keyboardFunc(unsigned char key, int x, int y)
{
  switch (key)
  {
    case 27: // ESC key
      exit(0); // exit the program
    break;

    case ' ':
      cout << "You pressed the spacebar." << endl;
    break;

    case 'x':
      // Take a screenshot.
      saveScreenshot("screenshot.jpg");
    break;
  }
}

// This is the main display function. It is called automatically by GLUT
// if you are familiar with Unity, this is similar to the Update() function but for rendering.
void displayFunc()
{
  // This function performs the actual rendering.

  // First, clear the screen. (and the depth buffer)
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

  // Set up the camera position, focus point, and the up vector.
  //This uses the OpenGLMatrix helper class to set up the view matrix.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.LoadIdentity();
  matrix.LookAt(0.0, 10.0, 5.0,
                0.0, 0.0, 0.0,
                0.0, 1.0, 0.0);
  // Apply interactive transforms (controlled by mouse) to the scene.
  // Use the helper functions on the OpenGLMatrix to compose the view/model matrix
  // from the terrainTranslate, terrainRotate and terrainScale variables.
  // Order: translate -> rotate X,Y,Z -> scale
  //Note: all of thse start at the identity.
  matrix.Translate(terrainTranslate[0], terrainTranslate[1], terrainTranslate[2]);
  matrix.Rotate(terrainRotate[0], 1.0f, 0.0f, 0.0f);
  matrix.Rotate(terrainRotate[1], 0.0f, 1.0f, 0.0f);
  matrix.Rotate(terrainRotate[2], 0.0f, 0.0f, 1.0f);
  matrix.Scale(terrainScale[0], terrainScale[1], terrainScale[2]);

  // Read the current modelview and projection matrices from our helper class.
  // The matrices are only read here; nothing is actually communicated to OpenGL yet.
  float modelViewMatrix[16]; // 16 floats, 4 by 4 homogenous matrix.
  matrix.SetMatrixMode(OpenGLMatrix::ModelView);
  matrix.GetMatrix(modelViewMatrix);

  float projectionMatrix[16];
  matrix.SetMatrixMode(OpenGLMatrix::Projection);
  matrix.GetMatrix(projectionMatrix);

  // Upload the modelview and projection matrices to the GPU. Note that these are "uniform" variables.
  // "uniforms" stay the same for all shader calls, 
  // if you want data sent to the GPU that is the same for each vertex, fragment, ect.
  // Important: these matrices must be uploaded to *all* pipeline programs used.
  // In this assignment, there is only one pipeline program, but is is not uncommon to have multiple pipeline programs in a single scene.
  // In such a case, you must separately upload to *each* pipeline program.
  // Important: do not make a typo in the variable name below; otherwise, the program will malfunction.
  pipelineProgram->Bind();
  pipelineProgram->SetUniformVariableMatrix4fv("modelViewMatrix", GL_FALSE, modelViewMatrix);
  pipelineProgram->SetUniformVariableMatrix4fv("projectionMatrix", GL_FALSE, projectionMatrix);

  // Execute the rendering.
  // Bind the VAO that we want to render. Remember, one object = one VAO. 
  vao->Bind();
  //THE DRAW CALL: this sends the current opengl state from the CPU to the GPU for rendering.
  //you may find swapping GL_TRIANGLES with GL_POINTS or GL_LINES useful for debugging. (see screenshots).
  glDrawArrays(GL_TRIANGLES, 0, numVertices);

  //Swap the double-buffers.
  //double buffering is a technique to avoid flickering and tearing when rendering animations.
  //we essentially use two framebuffers, one for displaying the current frame, and one for rendering the next frame.
  glutSwapBuffers();
}


// This is only called once at the beginning of the program.
// Your Procedural mesh grid should be here.
// You should also generate the noise texture here.

void initScene(int argc, char *argv[])
{


  // Set the background color.
  glClearColor(0.4f, 0.7f, 0.9f, 1.0f); // Blue color.

  // Enable z-buffering (i.e., hidden surface removal using the z-buffer algorithm).
  glEnable(GL_DEPTH_TEST);

  // Create a pipeline program. This operation must be performed BEFORE we initialize any VAOs.
  // A pipeline program contains our shaders. Different pipeline programs may contain different shaders.
  pipelineProgram = new PipelineProgram(); // Load and set up the pipeline program, including its shaders.
  // Load and set up the pipeline program, including its shaders.
  if (pipelineProgram->BuildShadersFromFiles(shaderBasePath, 
    "inkCommonVertex.glsl","inkCompositeFragment.glsl") != 0)
  {
    cout << "Failed to build the pipeline program." << endl;
    throw 1;
  } 
  cout << "Successfully built the pipeline program." << endl;

  // Bind the pipeline program that we just created. 
  // The purpose of binding a pipeline program is to activate the shaders that it contains, i.e.,
  // any object rendered from that point on, will use those shaders.
  // When the application starts, no pipeline program is bound, which means that rendering is not set up.
  // So, at some point (such as below), we need to bind a pipeline program.
  // From that point on, exactly one pipeline program is bound at any moment of time.
  pipelineProgram->Bind();

  const int noiseTextureSize = 256;
  generateGradientGrid();
  float ** noiseValues = createPerlinTexture(noiseTextureSize);
  noiseTexture = new Texture(noiseTextureSize, noiseTextureSize, noiseValues);
  noiseTexture->Bind();
  for (int y = 0; y < noiseTextureSize; ++y)
    free(noiseValues[y]);
  free(noiseValues);
  pipelineProgram->SetUniformVariablei("noiseTexture", 0);
  pipelineProgram->SetUniformVariablef("amplitude", 10.0f);
  pipelineProgram->Bind();

  const int gridRows = 50;
  const int gridColumns = 50;

  // A grid with N points has N - 1 cell intervals in that dimension.
  const int gridCellCount = (gridRows - 1) * (gridColumns - 1);
  // Each cell contains two triangles
  const int gridTriangleCount = gridCellCount * 2;
  // three VBO vertices per triangle
  const int gridVertexCount = gridTriangleCount * 3;

  cout << "Grid cell count: " << gridCellCount << endl;
  cout << "Grid triangle count: " << gridTriangleCount << endl;
  cout << "Grid vertex count: " << gridVertexCount << endl;

  //Here is an example of how to create a simple Quad with 2 triangles.
  //you will need much more for your terrain mesh.
  numVertices = gridVertexCount;
  const float desiredSize = 20.0f; // total width and length of the grid
  const float halfSize = desiredSize * 0.5f;

  pipelineProgram->SetUniformVariablef(
    "terrainSize",
    desiredSize
  );

  const float columnSpacing = desiredSize / (gridColumns - 1);
  const float rowSpacing = desiredSize / (gridRows - 1);

  float * gridPositions = new float[gridVertexCount * 3];
  int positionIndex = 0;

  int visitedCellCount = 0;

  for(int row = 0; row < gridRows - 1; ++row)
  {
    for(int column = 0; column < gridColumns - 1; ++column)
    {
      ++visitedCellCount;
      // Each cell spans the current and next row and column.
      const float x0 = -halfSize + column * columnSpacing;
      const float x1 = -halfSize + (column + 1) * columnSpacing;

      const float z0 = -halfSize + row * rowSpacing;
      const float z1 = -halfSize + (row + 1) * rowSpacing;

      // Triangle 1: p00, p10, p11
      gridPositions[positionIndex++] = x0;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z0;

      gridPositions[positionIndex++] = x0;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z1;

      gridPositions[positionIndex++] = x1;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z1;

      // Triangle 2: p00, p11, p01
      gridPositions[positionIndex++] = x0;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z0;

      gridPositions[positionIndex++] = x1;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z1;

      gridPositions[positionIndex++] = x1;
      gridPositions[positionIndex++] = 0.0f;
      gridPositions[positionIndex++] = z0;


      const bool isFirstCell = row ==0 && column == 0;
      const bool isLastCell = row == gridRows - 2 && column == gridColumns - 2;
      
      if (isFirstCell || isLastCell)
      {
        cout << "Cell (" << row << ", " << column << "): "
             << "x0=" << x0 << ", x1=" << x1
             << ", z0=" << z0 << ", z1=" << z1 << endl;
      }

    }
  }

  cout << "Floats written for complete grid: "
     << positionIndex << endl;


  

  // Create the VBOs. We make a separate VBO for vertices and colors.
  //GL_STATIC_DRAW means that the data in the VBO will not change, we are setting this once and then rendering it many times. If you were to change the data in the VBO, you would use GL_DYNAMIC_DRAW instead.
  vboVertices = new VBO(numVertices, 3, gridPositions, GL_STATIC_DRAW);// 3 values per position

  delete [] gridPositions;  
  gridPositions = nullptr;
  // Create the VAOs. There is a single VAO in this example.
  // Important: this code must be executed AFTER we created our pipeline program, and AFTER we set up our VBOs.
  // A VAO contains the geometry for a single object. There should be one VAO per object.
  vao = new VAO();

  // Set up the relationship between the "position" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  vao->ConnectPipelineProgramAndVBOAndShaderVariable(pipelineProgram, vboVertices, "position");

  // Set up the relationship between the "color" shader variable and the VAO.
  // Important: any typo in the shader variable name will lead to malfunction.
  // Check for any OpenGL errors.
  std::cout << "GL error status is: " << glGetError() << std::endl;
}


//main, you dont need to alter this
int main(int argc, char *argv[])
{
  srand(static_cast<unsigned int>(time(nullptr)));

  cout << "Initializing GLUT..." << endl;
  glutInit(&argc,argv);

  cout << "Initializing OpenGL..." << endl;

  #ifdef __APPLE__
    glutInitDisplayMode(GLUT_3_2_CORE_PROFILE | GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #else
    glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL);
  #endif

  glutInitWindowSize(windowWidth, windowHeight);
  glutInitWindowPosition(0, 0);
  glutCreateWindow(windowTitle);

  cout << "OpenGL Version: " << glGetString(GL_VERSION) << endl;
  cout << "OpenGL Renderer: " << glGetString(GL_RENDERER) << endl;
  cout << "Shading Language Version: " << glGetString(GL_SHADING_LANGUAGE_VERSION) << endl;

  #ifdef __APPLE__
    // This is needed on recent Mac OS X versions to correctly display the window.
    glutReshapeWindow(windowWidth - 1, windowHeight - 1);
  #endif

  // Tells GLUT to use a particular display function to redraw.
  glutDisplayFunc(displayFunc);
  // Perform animation inside idleFunc.
  glutIdleFunc(idleFunc);
  // callback for mouse drags
  glutMotionFunc(mouseMotionDragFunc);
  // callback for idle mouse movement
  glutPassiveMotionFunc(mouseMotionFunc);
  // callback for mouse button changes
  glutMouseFunc(mouseButtonFunc);
  // callback for resizing the window
  glutReshapeFunc(reshapeFunc);
  // callback for pressing the keys on the keyboard
  glutKeyboardFunc(keyboardFunc);

  // init glew
  #ifdef __APPLE__
    // nothing is needed on Apple
  #else
    // Windows, Linux
    GLint result = glewInit();
    if (result != GLEW_OK)
    {
      cout << "error: " << glewGetErrorString(result) << endl;
      exit(EXIT_FAILURE);
    }
  #endif

  // Perform the initialization.
  initScene(argc, argv);

  // Sink forever into the GLUT loop.
  glutMainLoop();
}
