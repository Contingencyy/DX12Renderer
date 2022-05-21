#pragma once

struct TextureDesc
{
	TextureDesc() {}
	TextureDesc(DXGI_FORMAT format, D3D12_RESOURCE_STATES initialState, D3D12_RESOURCE_FLAGS flags, uint32_t width, uint32_t height)
		: Format(format), InitialState(initialState), Flags(flags), Width(width), Height(height) {}

	DXGI_FORMAT Format;
	D3D12_RESOURCE_STATES InitialState;
	D3D12_RESOURCE_FLAGS Flags;

	uint32_t Width;
	uint32_t Height;
};

class Texture
{
public:
	Texture(const TextureDesc& textureDesc);
	~Texture();

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle();

	TextureDesc GetTextureDesc() const { return m_TextureDesc; }

	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();

private:
	TextureDesc m_TextureDesc = {};
	ComPtr<ID3D12Resource> m_d3d12Resource;

	D3D12_CPU_DESCRIPTOR_HANDLE m_DescriptorHandle = {};
	std::size_t m_AlignedBufferSize = 0;

};
