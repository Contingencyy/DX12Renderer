#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/ResourceSlotmap.h"

struct RenderSettings
{
	Resolution RenderResolution = { 1280, 720 };
	Resolution ShadowMapResolution = { 2048, 2048 };
	bool VSync = true;
};

struct RendererStatistics
{
	void Reset()
	{
		DrawCallCount = 0;
		TriangleCount = 0;
		MeshCount = 0;
	}

	uint32_t DrawCallCount = 0;
	uint32_t TriangleCount = 0;
	uint32_t MeshCount = 0;
};

struct Mesh
{
	RenderResourceHandle VertexBuffer;
	RenderResourceHandle IndexBuffer;

	RenderResourceHandle Material;
	BoundingBox BB;

	std::string DebugName;
};

struct Material
{
	RenderResourceHandle AlbedoTexture;
	RenderResourceHandle NormalTexture;
	RenderResourceHandle MetallicRoughnessTexture;

	float MetalnessFactor;
	float RoughnessFactor;

	TransparencyMode Transparency;
};

struct RenderState
{
	
	RenderSettings Settings;
	RendererStatistics Stats;

	static constexpr uint32_t MAX_MESH_INSTANCES = 500;
	static constexpr uint32_t MAX_MESHES = 500;
	static constexpr uint32_t MAX_MATERIALS = 500;
	static constexpr uint32_t MAX_DIR_LIGHTS = 1;
	static constexpr uint32_t MAX_POINT_LIGHTS = 50;
	static constexpr uint32_t MAX_SPOT_LIGHTS = 50;
	static constexpr uint32_t MAX_DEBUG_LINES = 50000;
	static constexpr uint32_t BACK_BUFFER_COUNT = 3;

	// Resource slotmaps
	ResourceSlotmap<Buffer> BufferSlotmap;
	ResourceSlotmap<Texture> TextureSlotmap;
	ResourceSlotmap<Mesh>MeshSlotmap;
	ResourceSlotmap<Material> MaterialSlotmap;

	// Render targets
	std::unique_ptr<Texture> DepthPrepassDepthTarget;
	std::unique_ptr<Texture> HDRColorTarget;
	std::unique_ptr<Texture> SDRColorTarget;

	// Buffers
	std::unique_ptr<Buffer> MeshInstanceBufferOpaque;
	std::unique_ptr<Buffer> MeshInstanceBufferTransparent;
	std::unique_ptr<Buffer> SceneDataConstantBuffer;
	std::unique_ptr<Buffer> MaterialConstantBuffer;
	std::unique_ptr<Buffer> LightConstantBuffer;
	std::unique_ptr<Buffer> TonemapConstantBuffer;
	std::unique_ptr<Buffer> DebugLineBuffer;

	// Default textures
	std::unique_ptr<Texture> DefaultWhiteTexture;
	std::unique_ptr<Texture> DefaultNormalTexture;

};

extern RenderState g_RenderState;
