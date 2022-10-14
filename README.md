# DX12Renderer
## Description
A 3D rendering and rasterization project written in C++17, using DirectX 12 as the graphics API. The renderer currently works with using almost all resources from a bindless descriptor heap, with the exception of constant buffers/constant data, which is set using either root descriptors or root constants. The bindless descriptor implementation uses the pre shader model 6.6 way, using descriptor table ranges for CBVs, SRVs, and UAVs respectively.
The project builds and runs on both Windows 10 and Windows 11.

## Features
- GLTF model/scene loading
- Bindless/bindful rendering
- Forward rendering
- Instancing
- Directional light/spotlights/pointlights (ambient and diffuse lighting)
- Geometric view frustum culling with points, spheres, and AABBs
- Tone mapping (Uncharted2, Linear, Reinhard, Filmic, ACES filmic)
- Render backend with platform agnostic renderers (debug and 3D renderer)
- GPU validation

## WIP
- Shadow mapping (directional lights, spotlights, pointlights)

## Planned features
- Mipmap generation
- Material system
- Full PBR
- G-Buffering
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

## Images
##### Directional light shadow mapping
![sponza_shadow_mapping_01](https://user-images.githubusercontent.com/34250026/194719281-e756dc51-b51b-430f-9ef6-9c5c2bb027f0.png)

##### Spotlight shadow mapping
![sponza_shadow_mapping_spotlights_01](https://user-images.githubusercontent.com/34250026/194946833-0f7060ee-cc2e-47cb-bba4-c654f76597b9.png)

##### Sponza (Reinhard tonemapping, directional light from above, white spotlights and colored pointlights)
![Sponza_Lights](https://user-images.githubusercontent.com/34250026/189538342-a83f89a1-bb30-4c4b-980a-2993196fe2c5.png)

##### Sponza (Directional lighting)
![Sponza_Directional_Lighting](https://user-images.githubusercontent.com/34250026/189538344-3ebbda4f-6d95-412d-868a-bd101b8096a0.png)

## Building
The project currently provides the Visual Studio 2022 solution file. CMake is currently not supported. You will need to have installed the latest Windows 10 SDK in the Visual Studio workloads.
