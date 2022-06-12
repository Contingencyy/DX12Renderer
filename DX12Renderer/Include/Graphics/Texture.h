#pragma once
#include "Graphics/DescriptorAllocation.h"

enum class TextureUsage
{
	TEXTURE_USAGE_SHADER_RESOURCE,
	TEXTURE_USAGE_RENDER_TARGET,
	TEXTURE_USAGE_DEPTH
};

enum class TextureFormat
{
	TEXTURE_FORMAT_RGBA8_UNORM,
	TEXTURE_FORMAT_DEPTH32
};

struct TextureDesc
{
	TextureDesc() = default;
	TextureDesc(TextureUsage usage, TextureFormat format, uint32_t width, uint32_t height)
		: Usage(usage), Format(format), Width(width), Height(height) {}

	TextureUsage Usage = TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE;
	TextureFormat Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;

	uint32_t Width = 1280;
	uint32_t Height = 720;
};

class Texture
{
public:
	Texture(const TextureDesc& textureDesc, const void* data);
	Texture(const TextureDesc& textureDesc);
	~Texture();

	void Resize(uint32_t width, uint32_t height);
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle();

	TextureDesc GetTextureDesc() const { return m_TextureDesc; }
	std::size_t GetByteSize() const { return m_ByteSize; }

	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();
	void CreateView();

private:
	TextureDesc m_TextureDesc = {};
	ComPtr<ID3D12Resource> m_d3d12Resource;

	DescriptorAllocation m_DescriptorAllocation = {};
	std::size_t m_ByteSize = 0;

};
