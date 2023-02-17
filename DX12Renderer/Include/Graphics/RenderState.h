#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/ResourceSlotmap.h"

struct RenderSettings
{
	Resolution RenderResolution = { 1280, 720 };
	Resolution ShadowMapResolution = { 2048, 2048 };

	bool EnableTAA = true;
	bool EnableVSync = true;
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

struct SceneData
{
	void Reset()
	{
		DirLightCount = 0;
		PointLightCount = 0;
		SpotLightCount = 0;
	}

	glm::mat4 ViewProjection = glm::identity<glm::mat4>();
	glm::vec3 CameraPosition = glm::vec3(0.0f);

	uint32_t DirLightCount = 0;
	uint32_t PointLightCount = 0;
	uint32_t SpotLightCount = 0;
};

enum class TonemapType : uint32_t
{
	LINEAR, REINHARD, UNCHARTED2, FILMIC, ACES_FILMIC,
	NUM_TYPES
};

static std::string TonemapTypeToString(TonemapType type)
{
	switch (type)
	{
	case TonemapType::LINEAR:
		return std::string("Linear");
	case TonemapType::REINHARD:
		return std::string("Reinhard");
	case TonemapType::UNCHARTED2:
		return std::string("Uncharted2");
	case TonemapType::FILMIC:
		return std::string("Filmic");
	case TonemapType::ACES_FILMIC:
		return std::string("ACES Filmic");
	default:
		return std::string("Unknown");
	}
}

enum class DebugShowTextureMode : uint32_t
{
	DEBUG_SHOW_TEXTURE_MODE_DEFAULT,
	DEBUG_SHOW_TEXTURE_MODE_VELOCITY,
	DEBUG_SHOW_TEXTURE_MODE_NUM_MODES
};

static std::string DebugShowTextureModeToString(DebugShowTextureMode mode)
{
	switch (mode)
	{
	case DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_DEFAULT:
		return std::string("Final output");
	case DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_VELOCITY:
		return std::string("Velocity");
	default:
		return std::string("Unknown");
	}
}

struct GlobalConstantBufferData
{
	// General render settings
	glm::mat4 ViewProj = glm::identity<glm::mat4>();
	glm::mat4 InvViewProj = glm::identity<glm::mat4>();

	glm::mat4 PrevViewProj = glm::identity<glm::mat4>();
	glm::mat4 PrevInvViewProj = glm::identity<glm::mat4>();

	Resolution Resolution = { 1280, 720 };

	// TAA settings
	glm::vec2 TAA_HaltonJitter = glm::vec2(0.500000f, 0.333333f);
	float TAA_SourceWeight = 0.05f;
	uint32_t TAA_NeighborhoodClamping = true;

	// Tonemapping settings
	DebugShowTextureMode PP_DebugShowTextureMode = DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_DEFAULT;
	float TM_Exposure = 1.5f;
	float TM_Gamma = 2.2f;
	TonemapType TM_Type = TonemapType::UNCHARTED2;
};

struct RenderState
{
	
	GlobalConstantBufferData GlobalCBData;
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

	// TAA History and TAA result
	std::unique_ptr<Texture> TAAResolveTarget;
	std::unique_ptr<Texture> TAAHistory;
	std::unique_ptr<Texture> VelocityTarget;

	// Buffers
	std::unique_ptr<Buffer> GlobalConstantBuffer;
	std::unique_ptr<Buffer> MeshInstanceBufferOpaque;
	std::unique_ptr<Buffer> MeshInstanceBufferTransparent;
	std::unique_ptr<Buffer> SceneDataConstantBuffer;
	std::unique_ptr<Buffer> MaterialConstantBuffer;
	std::unique_ptr<Buffer> LightConstantBuffer;
	std::unique_ptr<Buffer> DebugLineBuffer;

	// Default textures
	std::unique_ptr<Texture> DefaultWhiteTexture;
	std::unique_ptr<Texture> DefaultNormalTexture;

};

extern RenderState g_RenderState;

static const Texture& GetDebugShowDebugModeTexture(DebugShowTextureMode mode)
{
	switch (mode)
	{
	case DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_DEFAULT:
		return *g_RenderState.TAAResolveTarget;
	case DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_VELOCITY:
		return *g_RenderState.VelocityTarget;
	default:
		return *g_RenderState.TAAResolveTarget;
	}
}
