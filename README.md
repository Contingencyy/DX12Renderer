# DX12Renderer
## Description
A 3D rendering and rasterization project written in C++17, using DirectX 12 as the graphics API. The renderer currently works with using almost all resources from a bindless descriptor heap, with the exception of constant buffers/constant data, which is set using either root descriptors or root constants. The bindless descriptor implementation uses the pre shader model 6.6 way, using descriptor table ranges for CBVs, SRVs, and UAVs respectively.

The project builds and runs on both Windows 10 and Windows 11.

## Features
- GLTF model loading
- Bindless/bindful rendering
- Forward rendering
- Geometric view frustum culling with points, spheres, and AABBs
- Tone mapping (Uncharted2, Linear, Reinhard, Filmic, ACES filmic)
- GPU validation
- Directional light/spotlights/pointlights
- Shadow mapping (Multisampled PCF, slope-scaled bias)
- Mipmap generation
- Normal mapping
- Physically-based rendering
- Materials

## Planned features
- Multithreaded rendering/command list recording
- G-Buffering/Deferred rendering
- Temporal anti-aliasing
- Ambient occlusion
- Particle system
- PostFX (e.g. Bloom, Vignette, Chromatic abberation)

## Camera controls
- Forward/backward: W/S
- Left/right: A/D
- Up/down: SPACEBAR/LCTRL
- Faster movement: LSHIFT
- Rotate (yaw/pitch): Hold right mouse button + drag

## Additional controls
- Toggle VSync: V
- Toggle fullscreen: F11/ALT + ENTER
- Toggle GUI: F3

## Images
### Sponza (PBR, shadow mapping)
![Sponza_All_Lights](https://user-images.githubusercontent.com/34250026/208733401-02697f6f-75f1-460d-8cc3-a7b398a4c42a.png)
![Sponza_Helmet_PBR](https://user-images.githubusercontent.com/34250026/208733432-a8acb06a-0901-4687-9104-1852faaed82e.png)
![Sponza_Shadows](https://user-images.githubusercontent.com/34250026/208733436-40a987e2-5e99-4d49-a89a-959afe5269da.png)

### Normal mapping
![Normal_Mapping](https://user-images.githubusercontent.com/34250026/208733449-04d1e88d-862a-410c-99b1-fa0c97f4d4d0.png)
![Normal_Mapping_World_Space_Normals](https://user-images.githubusercontent.com/34250026/208733455-da6e5dde-fb2b-461e-94b5-07f3bcd94a3b.png)

## Building
The project currently provides the Visual Studio 2022 solution file. CMake is currently not supported. You will need to have installed the latest Windows 10 SDK in the Visual Studio workloads.
