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

class RenderBackend
{
public:
	static void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	static void BeginFrame();
	static void OnImGuiRender();
	static void EndFrame();
	static void Finalize();

	static void CreateBuffer(ComPtr<ID3D12Resource>& d3d12Resource, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC& bufferDesc, D3D12_RESOURCE_STATES initialState);
	static void CreateTexture(ComPtr<ID3D12Resource>& d3d12Resource, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue);

	static void UploadBufferData(Buffer& destBuffer, const void* bufferData);
	static void UploadBufferDataRegion(Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes);
	static void UploadTexture(Texture& destTexture, const void* textureData);

	static ID3D12QueryHeap* GetD3D12TimestampQueryHeap();
	static const Buffer& GetQueryReadbackBuffer(uint32_t backBufferIndex);
	static void TrackTimestampQueryResult(const std::string& name, const TimestampQuery& timestampQuery, uint32_t backBufferIndex);
	static uint32_t GetMaxQueriesPerFrame();
	static uint32_t GetNextQueryIndex(uint32_t backBufferIndex);

	static void Resize(uint32_t width, uint32_t height);
	static void Flush();
	static void SetVSync(bool vSync);

	static IDXGIAdapter4* GetDXGIAdapter();
	static ID3D12Device2* GetD3D12Device();
	static SwapChain& GetSwapChain();

	static DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
	static DescriptorHeap& GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

	static std::shared_ptr<CommandList> GetCommandList(D3D12_COMMAND_LIST_TYPE type);
	static void ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	static void ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList);

private:
	static void EnableDebugLayer();
	static void CreateAdapter();
	static void CreateDevice();

	static void ProcessTimestampQueries();

};
