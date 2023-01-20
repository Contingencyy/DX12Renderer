#pragma once
#include "Scene/BoundingVolume.h"

struct RenderResourceHandle
{
	union
	{
		uint64_t Handle;
		struct
		{
			uint32_t Index;
			uint32_t Version;
		};
	};
};

#define RENDER_RESOURCE_HANDLE_NULL RenderResourceHandle{}
#define RENDER_RESOURCE_HANDLE_VALID(handle) (handle.Index != 0)

struct RenderSettings
{
	struct Resolution
	{
		uint32_t x = 1280;
		uint32_t y = 720;
	};

	Resolution RenderResolution;
	Resolution ShadowMapResolution = { 2048, 2048 };
	bool VSync = true;
};

enum class BufferUsage : uint32_t
{
	BUFFER_USAGE_NONE = 0,
	BUFFER_USAGE_VERTEX = (1 << 0),
	BUFFER_USAGE_INDEX = (1 << 1),
	BUFFER_USAGE_READ = (1 << 2),
	BUFFER_USAGE_WRITE = (1 << 3),
	BUFFER_USAGE_CONSTANT = (1 << 4),
	BUFFER_USAGE_UPLOAD = (1 << 5),
	BUFFER_USAGE_READBACK = (1 << 6),
};

inline bool operator&(BufferUsage lhs, BufferUsage rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}

inline BufferUsage operator|(BufferUsage lhs, BufferUsage rhs)
{
	return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

struct BufferDesc
{
	BufferUsage Usage;
	std::size_t NumElements;
	std::size_t ElementSize;
	const void* DataPtr;

	std::string DebugName;
};

enum class TextureUsage : uint32_t
{
	TEXTURE_USAGE_NONE = 0,
	TEXTURE_USAGE_READ = (1 << 0),
	TEXTURE_USAGE_WRITE = (1 << 1),
	TEXTURE_USAGE_RENDER_TARGET = (1 << 2),
	TEXTURE_USAGE_DEPTH = (1 << 3)
};

inline bool operator&(TextureUsage lhs, TextureUsage rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}

inline TextureUsage operator|(TextureUsage lhs, TextureUsage rhs)
{
	return static_cast<TextureUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

enum class TextureFormat : uint32_t
{
	TEXTURE_FORMAT_UNSPECIFIED = 0,
	TEXTURE_FORMAT_RGBA8_UNORM,
	TEXTURE_FORMAT_RGBA8_SRGB,
	TEXTURE_FORMAT_RGBA16_FLOAT,
	TEXTURE_FORMAT_DEPTH32
};

enum class TextureDimension : uint32_t
{
	TEXTURE_DIMENSION_UNSPECIFIED = 0,
	TEXTURE_DIMENSION_2D,
	TEXTURE_DIMENSION_CUBE
};

struct TextureDesc
{
	TextureUsage Usage;
	TextureFormat Format;
	TextureDimension Dimension;

	uint32_t Width;
	uint32_t Height;
	uint32_t NumMips;

	union
	{
		glm::vec4 ClearColor;
		glm::vec2 ClearDepthStencil;
	};

	const void* DataPtr;

	std::string DebugName;
};

enum TransparencyMode : uint32_t
{
	OPAQUE = 0, TRANSPARENT = 1,
	NUM_ALPHA_MODES = 2
};

struct Material
{
public:
	RenderResourceHandle AlbedoTexture;
	RenderResourceHandle NormalTexture;
	RenderResourceHandle MetallicRoughnessTexture;

	float MetalnessFactor;
	float RoughnessFactor;

	TransparencyMode Transparency;
}; 

struct MeshPrimitive
{
	Material Material;
	BoundingBox BB;

	std::size_t VerticesStart;
	std::size_t IndicesStart;
	std::size_t NumIndices;

	RenderResourceHandle VertexBuffer;
	RenderResourceHandle IndexBuffer;

	std::string Name;
};

struct Mesh
{
	std::vector<MeshPrimitive> Primitives;
	std::string Name;
	std::size_t Hash;
};
