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
#define RENDER_RESOURCE_HANDLE_COMPARE(lhandle, rhandle) ((lhandle).Index == (rhandle).Index)

struct Resolution
{
	uint32_t x = 0;
	uint32_t y = 0;
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
	BufferUsage Usage = BufferUsage::BUFFER_USAGE_NONE;
	std::size_t NumElements = 0;
	std::size_t ElementSize = 0;
	const void* DataPtr = nullptr;

	std::string DebugName = "Unnamed";
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
	TEXTURE_FORMAT_RG16_FLOAT,
	TEXTURE_FORMAT_DEPTH32
};

enum class TextureDimension : uint32_t
{
	TEXTURE_DIMENSION_2D = 0,
	TEXTURE_DIMENSION_CUBE
};

struct TextureDesc
{
	TextureUsage Usage = TextureUsage::TEXTURE_USAGE_NONE;
	TextureFormat Format = TextureFormat::TEXTURE_FORMAT_UNSPECIFIED;
	TextureDimension Dimension = TextureDimension::TEXTURE_DIMENSION_2D;

	uint32_t Width = 0;
	uint32_t Height = 0;
	uint32_t NumMips = 1;

	union
	{
		glm::vec4 ClearColor = glm::vec4(0.0f, 0.0f, 0.0f, 1.0f);
		glm::vec2 ClearDepthStencil;
	};

	const void* DataPtr = nullptr;

	std::string DebugName = "Unnamed";
};

enum TransparencyMode : uint32_t
{
	OPAQUE = 0, TRANSPARENT = 1,
	NUM_ALPHA_MODES = 2
};

struct MeshDesc
{
	BufferDesc VertexBufferDesc;
	BufferDesc IndexBufferDesc;

	RenderResourceHandle MaterialHandle;
	BoundingBox BB;

	std::string DebugName;
};

struct MaterialDesc
{
	TextureDesc AlbedoDesc;
	TextureDesc NormalDesc;
	TextureDesc MetallicRoughnessDesc;

	float Metalness;
	float Roughness;

	TransparencyMode Transparency;

	std::string DebugName;
};
