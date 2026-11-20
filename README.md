# CSCI 580 Chinses Ink NPR
Team Members: 
- Allan Tsai awtsai@usc.edu
- Nathan Liu liunatha@usc.edu
- Jiarui Peng jiaruipe@usc.edu
- Yinan Zhang yinanz56@usc.edu

## Overview (Proposal)
Our project aims to make a real-time non-photorealistic rendering that transforms 3D scenes/objects into the visual style of traditional Chinese ink and wash paintings. The project involves 3D transformations, rasterization, lighting, texture mapping, depth testing, and programmable shaders. We will use custom GPU shaders to create these specific effects, apply the knowledge from Lecture 6 to generate ink-wash color gradients by modifying Phong diffuse lighting and Lecture 8 to add noise for creating brushstrokes and rice paper textures. We will also build on techniques from the OpenGL Terrain assignment, including model transformations, surface-normal calculations, lighting, procedural noise, and interactive camera controls. The new topics we need to learn include non-photorealistic rendering, view-dependent contour-line detection, irregular ink-tone generation, and simple screen-space ink diffusion. Our primary technical references would be from Li and Li [1] that focus on animal models, utilizes controllable contour lines, quantized diffuse shading, noise mapping, and paper texture compositing. Our visual style is inspired by the game TENSEI [3]. According to an interview with its development team, the game’s environmental assets were modeled, then covered with hand-painted ink-wash textures, and then rendered with Unity Shader Graph[2]. In contrast, our project will focus on generating the ink-wash appearance procedurally in real time from 3D geometry, surface normals, lighting, textures, and the camera’s viewing direction using custom OpenGL/GLSL shaders. The final program will allow users to move the camera, adjust various ink-wash rendering parameters, and switch between traditional Phong rendering and Chinese ink-wash style rendering.

Currently in progress (11/14)
<img width="1292" height="756" alt="image" src="https://github.com/user-attachments/assets/81e48ae2-47c1-433b-a70a-28274f13d7a2" />



## Extra Features (In Progress)

We will try different NPR methods for the ink paintings and come up with our own method that mimics these styles, compare and contrast to the Li and Li [1]'s papers.

## Expected Code and Middleware

- C++ for application logic, scene management, camera controls, and render-pass management.
- OpenGL for real-time mesh rendering and framebuffer-based post-processing.
- GLSL vertex and fragment shaders for transformations, lighting, contour detection, ink-tone quantization, texture sampling, and final compositing.
- Multiple render passes and framebuffer objects for ink diffusion, edge processing, and paper-texture compositing.
- GLEW for access to OpenGL functionality.
- FreeGLUT for window creation and keyboard/mouse input.
- GLM or the course-provided matrix/vector classes for transformations and vector calculations.
- The course-provided OBJ loader, or a small OBJ-loading library, for importing the animal model.
- `stb_image` or the course ImageIO library for loading noise, brush, and rice-paper textures.
- Blender, if necessary, for inspecting, simplifying, or preparing 3D assets; Blender will not perform the final runtime rendering.
- Git/GitHub for version control and collaboration.


## Controls
The following camera controls are provided by the starter code:

- Left mouse drag: Rotate the camera around the X and Y axes.
- Middle mouse drag: Rotate around the Z axis.
- Ctrl + left mouse drag: Translate along the X and Y axes.
- Ctrl + middle mouse drag: Translate along the Z axis.
- Shift + left mouse drag: Scale along the X and Y axes.
- Shift + middle mouse drag: Scale along the Z axis.
- X key: Save a screenshot.
- P key: Toggle the paper, brush, and ink diffusion effects.
- Esc key: Exit the program.

## Build and Run

On Ubuntu or WSL, open a terminal in the project root folder (`CSCI580-Chinese-Ink-NPR`) and run:

```
cd Terrain
make
./hw1
```

To preview a local OBJ instead of procedural terrain, pass its path at launch.

```sh
./hw1 --obj "/path/to/model.obj"
```

On Windows, run the following from the project root after building Debug / Win32.

```powershell
& ".\Terrain\Bin\Debug\OpenGL Terrain.exe" --obj "C:\path\to\model.obj"
```

The preview centers and scales the mesh to fit the camera. It uses the project's
ink shader for shading and accepts OBJ geometry, normals, and UVs. Provide a local
OBJ file when running this command.

## Submission Media (In Progress)
`FinalPresentation.mp4`: An OBS-recorded video showing the completed NPR effects from different camera views.

## Relevant Sources
[1] F. Li and S. Li, “Real-time Rendering of 3D Animal Models in Chinese Ink Painting Style,” 2020 International Conference on Culture-oriented Science & Technology (ICCST), Beijing, China, 2020, pp. 284–287. https://doi.org/10.1109/ICCST50977.2020.00061

[2] Dengeki Online, “350円で買える禅アクション『点睛（TENSEI）』インタビュー。水墨画風のデザイン、操作キャラが目になった経緯、エンドレスに遊べる中毒性の秘密を紹介,” July 11, 2024. Available: https://dengekionline.com/article/202407/8658.

[3] Project Pegasus, TENSEI [Video game]. NEURON-AGE, 2024. Available: https://store.steampowered.com/app/2482150/TENSEI/


