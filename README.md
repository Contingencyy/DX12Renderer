# DX12Renderer
## Description
A 3D rendering project written in C++, using DirectX 12 as the graphics API.

## Features
- GLTF model/scene loading
- Bindful rendering
- ~~Instancing~~ (currently broken)
- Albedo and normal textures
- Geometric view frustum culling with points, spheres, and AABBs
- Point lights (ambient and diffuse)
- Tone mapping (Uncharted2, Linear, Reinhard, Filmic, ACES filmic)
- Render backend with platform agnostic renderers

## Planned features
- Directional light/spotlights
- Bindless rendering
- Full PBR
- Material system
- Mipmap generation
- MSAA
- Shadow mapping
- Ambient occlusion
- Particle system
- Post-process: Bloom

## Camera controls
- Forward/backward: W/S
- Left/right: A/D
- Up/down: LSHIFT/LCTRL
- Rotate (yaw/pitch): Hold right mouse button + drag

## Images
##### Instancing
![Rendering instanced models](https://user-images.githubusercontent.com/34250026/173068461-ad322038-f782-4ab7-a98d-2fb115ddfd78.png)

##### Point lights (ambient and diffuse)
![PointLightsAmbientDiffuse](https://user-images.githubusercontent.com/34250026/173195505-a01fbea1-0427-4e13-910f-60f886b4678b.png)

##### Tone mapping (Uncharted2)
![Tonemapping](https://user-images.githubusercontent.com/34250026/182204514-52543369-4537-464a-8929-8b68596bbd3a.png)

##### Sponza (Reinhard tonemapping, ambient and diffuse point lights)
![Sponza](https://user-images.githubusercontent.com/34250026/187080988-e7179a6e-4b41-42ec-ac40-00640270c7bc.png)

##### Sponza (with bounding boxes of individual meshes for view frustum culling)
![Sponza_Bounding_Boxes](https://user-images.githubusercontent.com/34250026/187080993-3966b143-f44c-4148-b451-e3560f8e7d28.png)

## Building
The project currently provides the Visual Studio 2019 solution file. CMake is currently not supported. You will need to have installed the latest Windows 10 SDK.
