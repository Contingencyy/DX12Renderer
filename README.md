# DX12Renderer
## Description
A 3D rendering project written in C++, using DirectX 12 as the graphics API.

## Features
- GLTF model loading (single mesh only)
- Instancing
- Albedo and normal textures
- Geometric view frustum culling with points and spheres
- Point lights (ambient and diffuse)
- Tone mapping (Uncharted2, Linear, Reinhard, Filmic, ACES filmic)

## Images
##### Instancing
![Rendering instanced models](https://user-images.githubusercontent.com/34250026/173068461-ad322038-f782-4ab7-a98d-2fb115ddfd78.png)

##### Point lights (ambient and diffuse)
![PointLightsAmbientDiffuse](https://user-images.githubusercontent.com/34250026/173195505-a01fbea1-0427-4e13-910f-60f886b4678b.png)

##### Tone mapping ()
![Tonemapping](https://user-images.githubusercontent.com/34250026/182204514-52543369-4537-464a-8929-8b68596bbd3a.png)

## Building
The project currently provides the Visual Studio 2019 solution file. CMake is currently not supported. You will need to have installed the latest Windows 10 SDK.
