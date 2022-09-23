#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"

class Device
{
public:
	Device();
	~Device();

	void CreateCommandQueue(CommandQueue& commandQueue, const D3D12_COMMAND_QUEUE_DESC& queueDesc);
	void CreateCommandList(CommandList& commandList);
	void CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc, ComPtr<ID3D12DescriptorHeap>& heap);

	void CreatePipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc, ComPtr<ID3D12PipelineState>& pipelineState);
	void CreateRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc, ComPtr<ID3D12RootSignature>& rootSignature);

	void CreateBuffer(Buffer& buffer, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size);
	void CreateTexture(Texture& texture, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue = nullptr);

	void CreateRenderTargetView(Texture& texture, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateDepthStencilView(Texture& texture, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateConstantBufferView(Buffer& buffer, const D3D12_CONSTANT_BUFFER_VIEW_DESC& cbvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateShaderResourceView(Texture& texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);
	void CreateUnorderedAccessView(Texture& texture, const D3D12_UNORDERED_ACCESS_VIEW_DESC& uavDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor);

	uint32_t GetDescriptorIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type);
	void CopyDescriptors(uint32_t numDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* destDescriptorRangeStarts, const uint32_t* destDescriptorRangeSizes,
		uint32_t numSrcDescriptorRanges, const D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorRangeStarts, const uint32_t* srcDescriptorRangeSizes, D3D12_DESCRIPTOR_HEAP_TYPE type);

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
