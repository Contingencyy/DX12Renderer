#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"
#include "Graphics/Buffer.h";

class SwapChain;
class DescriptorHeap;
class CommandQueue;
class CommandList;
class Texture;

struct TimestampQuery
{
	void ReadFromBuffer(const Buffer& queryResultBuffer)
	{
		BeginQueryTimestamp = queryResultBuffer.ReadBackBytes<uint64_t>(sizeof(uint64_t), StartIndex * sizeof(uint64_t));
		EndQueryTimestamp = queryResultBuffer.ReadBackBytes<uint64_t>(sizeof(uint64_t), (StartIndex + 1) * sizeof(uint64_t));
	}

	uint64_t GetTicks()
	{
		return (EndQueryTimestamp - BeginQueryTimestamp);
	}

	uint32_t StartIndex = 0;
	uint32_t NumQueries = 0;

	uint64_t BeginQueryTimestamp = 0;
	uint64_t EndQueryTimestamp = 0;
};

namespace RenderBackend
{

	void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	void BeginFrame();
	void OnImGuiRender();
	void EndFrame();
	void Finalize();
	void CreateBuffer(ComPtr<ID3D12Resource>& d3d12Resource, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC& bufferDesc, D3D12_RESOURCE_STATES initialState);
	void CreateTexture(ComPtr<ID3D12Resource>& d3d12Resource, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue);
	void UploadBufferData(Buffer& destBuffer, const void* bufferData);
	void UploadBufferDataRegion(Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes);
	void UploadTextureData(Texture& destTexture, const void* textureData);
	void GenerateMips(Texture& texture);

	ID3D12QueryHeap* GetD3D12TimestampQueryHeap();
	const Buffer& GetQueryReadbackBuffer(uint32_t backBufferIndex);
	void TrackTimestampQueryResult(const std::string& name, const TimestampQuery& timestampQuery, uint32_t backBufferIndex);
	uint32_t GetMaxQueriesPerFrame();
	uint32_t GetNextQueryIndex(uint32_t backBufferIndex);

	void Resize(uint32_t width, uint32_t height);
	void Flush();
	void SetVSync(bool vSync);

	IDXGIAdapter4* GetDXGIAdapter();
	ID3D12Device2* GetD3D12Device();
	SwapChain& GetSwapChain();
	ID3D12PipelineState* GetMipGenPSO();
	ID3D12RootSignature* GetMipGenRootSig();
	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
	DescriptorHeap& GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

	std::shared_ptr<CommandList> GetCommandList(D3D12_COMMAND_LIST_TYPE type);
	void ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	void ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList);

};
