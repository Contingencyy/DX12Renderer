#pragma once
#include "Graphics/DescriptorAllocation.h"

struct TextureDesc
{
	TextureDesc() {}
	TextureDesc(DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, uint32_t width, uint32_t height)
		: Format(format), InitialState(initialState), Flags(flags), Width(width), Height(height) {}

	DXGI_FORMAT Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
	D3D12_RESOURCE_FLAGS Flags = D3D12_RESOURCE_FLAG_NONE;

	uint32_t Width;
	uint32_t Height;
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
