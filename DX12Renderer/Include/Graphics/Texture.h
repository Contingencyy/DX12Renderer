#pragma once
#include "Graphics/DescriptorAllocation.h"

enum class TextureUsage
{
	TEXTURE_USAGE_NONE,
	TEXTURE_USAGE_SHADER_RESOURCE,
	TEXTURE_USAGE_BACK_BUFFER,
	TEXTURE_USAGE_RENDER_TARGET,
	TEXTURE_USAGE_DEPTH
};

enum class TextureFormat
{
	TEXTURE_FORMAT_UNSPECIFIED,
	TEXTURE_FORMAT_RGBA8_UNORM,
	TEXTURE_FORMAT_RGBA16_FLOAT,
	TEXTURE_FORMAT_DEPTH32
};

struct TextureDesc
{
	TextureDesc() = default;
	TextureDesc(TextureUsage usage, TextureFormat format, uint32_t width, uint32_t height)
		: Usage(usage), Format(format), Width(width), Height(height) {}

	TextureUsage Usage = TextureUsage::TEXTURE_USAGE_NONE;
	TextureFormat Format = TextureFormat::TEXTURE_FORMAT_UNSPECIFIED;

	uint32_t Width = 1;
	uint32_t Height = 1;
};

DXGI_FORMAT TextureFormatToDXGIFormat(TextureFormat format);

class Texture
{
public:
	Texture(const std::string& name, const TextureDesc& textureDesc, const void* data);
	Texture(const std::string& name, const TextureDesc& textureDesc);
	~Texture();

	void Resize(uint32_t width, uint32_t height);

	D3D12_CPU_DESCRIPTOR_HANDLE GetRenderTargetDepthStencilView();
	D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView();
	D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView();

	uint32_t GetSRVIndex() const;
	uint32_t GetUAVIndex() const;

	TextureDesc GetTextureDesc() const { return m_TextureDesc; }
	std::size_t GetByteSize() const { return m_ByteSize; }

	std::string GetName() const { return m_Name; }
	void SetName(const std::string& name);
	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();
	void CreateViews();

private:
	TextureDesc m_TextureDesc = {};
	std::string m_Name = "";
	ComPtr<ID3D12Resource> m_d3d12Resource;

	DescriptorAllocation m_RenderTargetDepthStencilDescriptor = {};
	DescriptorAllocation m_ShaderResourceViewDescriptor = {};
	DescriptorAllocation m_UnorderedAccessViewDescriptor = {};

	std::size_t m_ByteSize = 0;

};
