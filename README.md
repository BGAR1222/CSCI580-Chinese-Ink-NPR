# CSCI 580 Terrain Generation
USC username: jiaruipe

## Overview
The program generates a procedural mountain terrain.
The CPU generates a fractal Perlin noise texture.
The vertex shader uses the texture for displacement, and the fragment shader creates 4 height-based colors.
I used bright pink, purple, yellow, and orange colors to give the terrain a psychedelic appearance.

## Features
- A 50 x 50 grid mesh rendered as 4,802 triangles.
- A 256 x 256 fractal Perlin noise texture generated on the CPU.
- The Vertex Shader uses the texture to displace terrain vertices.
- The Fragment Shader creates four height-based colors.
- The fractal noise combines four octaves to add smaller terrain details.

## Extra Feature
The vertex shader samples neighboring height values to calculate the terrain slope.
2 tangent vectors and a cross product are used to derive the surface normal.
The normal is passed to the fragment shader, which applies ambient, diffuse, and specular lighting using Phong shading.

## Controls
The following camera controls are provided by the starter code:

- Left mouse drag: Rotate the camera around the X and Y axes.
- Middle mouse drag: Rotate around the Z axis.
- Ctrl + left mouse drag: Translate along the X and Y axes.
- Ctrl + middle mouse drag: Translate along the Z axis.
- Shift + left mouse drag: Scale along the X and Y axes.
- Shift + middle mouse drag: Scale along the Z axis.
- X key: Save a screenshot.
- Esc key: Exit the program.

## Build and Run
This project was built on Windows using Visual Studio 2022 with the v143 platform toolset.

1. Open a PowerShell terminal in the `Terrain` folder.
2. Build the Debug Win32 configuration:
   `& "C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe" ".\terrain.sln" /t:Build /p:Configuration=Debug /p:Platform=Win32`
3. Run the compiled program:
   `& ".\Bin\Debug\OpenGL Terrain.exe"`

## Important Parameters
- Grid size: 50 x 50
- Noise texture size: 256 x 256
- Number of octaves: 4
- Base frequency: 4.0
- Lacunarity: 2.0
- Persistence: 0.5
- Terrain amplitude: 10.0

## Submission Media
`FinalTerrain.mp4`: An OBS-recorded video showing the completed terrain and its Phong shading from different camera views.

## Known Issues
- No known runtime issues.
- The Windows build may display linker warnings related to the provided JPEG library, but it completes successfully with zero errors.