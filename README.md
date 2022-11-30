# DX12Renderer
## Description
A 3D rendering and rasterization project written in C++17, using DirectX 12 as the graphics API. The renderer currently works with using almost all resources from a bindless descriptor heap, with the exception of constant buffers/constant data, which is set using either root descriptors or root constants. The bindless descriptor implementation uses the pre shader model 6.6 way, using descriptor table ranges for CBVs, SRVs, and UAVs respectively.

The project builds and runs on both Windows 10 and Windows 11.

## Features
- GLTF model loading
- Bindless/bindful rendering
- Forward rendering
- Instancing
- Geometric view frustum culling with points, spheres, and AABBs
- Tone mapping (Uncharted2, Linear, Reinhard, Filmic, ACES filmic)
- Render backend with platform agnostic renderers (debug and 3D renderer)
- GPU validation
- Directional light/spotlights/pointlights (ambient and diffuse lighting)
- Shadow mapping (Multisampled PCF, slope-scaled bias)
- Mipmap generation

## Planned features
- Multithreaded rendering/command list recording
- Material system
- Full PBR
- G-Buffering/Deferred rendering
- MSAA
- Ambient occlusion
- Particle system
- Post-process: Bloom, Vignette

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
### Sponza
![Sponza_Dirlights_Spotlights](https://user-images.githubusercontent.com/34250026/204929516-e82bc7eb-7846-4d31-b155-ed8126a7214f.png)
![Sponza_Dirlights_Spotlights_Pointlights](https://user-images.githubusercontent.com/34250026/204929518-925c8b83-4dc4-4613-9422-71674364be73.png)

### Shadow mapping
![Sponza_Pointlight_Shadows](https://user-images.githubusercontent.com/34250026/204929501-04e5c15a-c799-472d-b818-32d274a221c4.png)
![Sponza_Spotlight_Shadow](https://user-images.githubusercontent.com/34250026/204929504-9820795c-d5c1-4ae3-b8de-22d14b7cd09c.png)

### GUI
![Sponza_GUI_Showcase](https://user-images.githubusercontent.com/34250026/204929523-4dac0a28-d761-4472-a37e-a500647324f3.png)

## Building
The project currently provides the Visual Studio 2022 solution file. CMake is currently not supported. You will need to have installed the latest Windows 10 SDK in the Visual Studio workloads.
