#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"

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
	TEXTURE_FORMAT_RGBA16_FLOAT,
	TEXTURE_FORMAT_DEPTH32
};

enum class TextureDimension : uint32_t
{
	TEXTURE_DIMENSION_2D = 0,
	TEXTURE_DIMENSION_CUBE
};

struct TextureDesc
{
	TextureDesc() = default;
	TextureDesc(TextureUsage usage, TextureFormat format, uint32_t width, uint32_t height)
		: Usage(usage), Format(format), Width(width), Height(height) {}
	TextureDesc(TextureUsage usage, TextureFormat format, TextureDimension dimension, uint32_t width, uint32_t height)
		: Usage(usage), Format(format), Dimension(dimension), Width(width), Height(height) {}

	TextureUsage Usage = TextureUsage::TEXTURE_USAGE_NONE;
	TextureFormat Format = TextureFormat::TEXTURE_FORMAT_UNSPECIFIED;
	TextureDimension Dimension = TextureDimension::TEXTURE_DIMENSION_2D;

	uint32_t Width = 1;
	uint32_t Height = 1;

	glm::vec4 ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
	glm::vec2 ClearDepthStencil = glm::vec2(1.0f, 0.0f);
};

DXGI_FORMAT TextureFormatToDXGIFormat(TextureFormat format);
D3D12_RESOURCE_DIMENSION TextureDimensionToD3DDimension(TextureDimension dimension);
D3D12_SRV_DIMENSION TextureDimensionToD3DSRVDimension(TextureDimension dimension);

class Texture
{
public:
	Texture(const std::string& name, const TextureDesc& textureDesc, const void* data);
	Texture(const std::string& name, const TextureDesc& textureDesc);
	~Texture();

	void Resize(uint32_t width, uint32_t height);
	bool IsValid() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(DescriptorType type) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCubeDepthDescriptorHandle(DescriptorType type, uint32_t face) const;
	uint32_t GetDescriptorIndex(DescriptorType type) const;

	const TextureDesc& GetTextureDesc() const { return m_TextureDesc; }
	std::size_t GetByteSize() const { return m_ByteSize; }
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name);

	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

protected:
	void Create();
	void CreateViews();

protected:
	TextureDesc m_TextureDesc = {};
	std::string m_Name = "";
	ComPtr<ID3D12Resource> m_d3d12Resource;

	DescriptorAllocation m_DescriptorAllocations[DescriptorType::NUM_DESCRIPTOR_TYPES] = {};
	DescriptorAllocation m_CubeDescriptorAllocations[DescriptorType::NUM_DESCRIPTOR_TYPES] = {};

	std::size_t m_ByteSize = 0;

};
