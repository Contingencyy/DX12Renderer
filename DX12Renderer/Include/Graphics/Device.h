#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

class Device
{
public:
	Device();
	~Device();
	
	void CreateBuffer(Buffer& buffer, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size);
	void CreateTexture(Texture& texture, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	void CreateRenderTargetView(Texture& texture, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateDepthStencilView(Texture& texture, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateShaderResourceView(Texture& texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	ComPtr<IDXGIAdapter4> GetDXGIAdapter() const { return m_dxgiAdapter; }
	ComPtr<ID3D12Device2> GetD3D12Device() const { return m_d3d12Device; }

private:
	void EnableDebugLayer();
	void CreateAdapter();
	void CreateDevice();

private:
	ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	ComPtr<ID3D12Device2> m_d3d12Device;

};
