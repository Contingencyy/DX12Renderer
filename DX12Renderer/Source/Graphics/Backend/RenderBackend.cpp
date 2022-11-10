#include "Pch.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/CommandQueue.h"

#include <imgui/imgui.h>

struct InternalRenderBackendData
{
	std::shared_ptr<Device> Device;
	std::unique_ptr<SwapChain> SwapChain;
	std::unique_ptr<DescriptorHeap> DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	ComPtr<ID3D12QueryHeap> D3D12QueryHeapTimestamp;
	std::unique_ptr<Buffer> QueryResultBuffers[3];
	uint32_t MaxQueriesPerFrame = 32;
	uint32_t NextQueryIndex[3] = { 0, 0, 0 };

	std::shared_ptr<CommandQueue> CommandQueueDirect;
	std::unique_ptr<CommandQueue> CommandQueueCompute;
	std::unique_ptr<CommandQueue> CommandQueueCopy;

	std::thread ProcessInFlightCommandListsThread;
	std::atomic_bool ProcessInFlightCommandLists;

	std::unordered_map<std::string, TimestampQuery> TimestampQueries[3];
	uint32_t CurrentBackBufferIndex = 0;

	bool VSync = true;
};

static InternalRenderBackendData s_Data;

void RenderBackend::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
	s_Data.Device = std::make_shared<Device>();

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		s_Data.DescriptorHeaps[i] = std::make_unique<DescriptorHeap>(s_Data.Device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

	s_Data.CommandQueueDirect = std::make_shared<CommandQueue>(s_Data.Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	s_Data.CommandQueueCompute = std::make_unique<CommandQueue>(s_Data.Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	s_Data.CommandQueueCopy = std::make_unique<CommandQueue>(s_Data.Device, D3D12_COMMAND_LIST_TYPE_COPY);

	s_Data.SwapChain = std::make_unique<SwapChain>(hWnd, s_Data.CommandQueueDirect, width, height);

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	queryHeapDesc.Count = s_Data.MaxQueriesPerFrame * s_Data.SwapChain->GetBackBufferCount();
	queryHeapDesc.NodeMask = 0;
	DX_CALL(s_Data.Device->GetD3D12Device()->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&s_Data.D3D12QueryHeapTimestamp)));

	for (uint32_t i = 0; i < s_Data.SwapChain->GetBackBufferCount(); ++i)
	{
		s_Data.QueryResultBuffers[i] = std::make_unique<Buffer>("Query result buffer", BufferDesc(BufferUsage::BUFFER_USAGE_READBACK,
			s_Data.MaxQueriesPerFrame, 8));
	}

	s_Data.ProcessInFlightCommandLists = true;
	s_Data.ProcessInFlightCommandListsThread = std::thread([]() {
		while (s_Data.ProcessInFlightCommandLists)
		{
			s_Data.CommandQueueDirect->ResetCommandLists();
			s_Data.CommandQueueCompute->ResetCommandLists();
			s_Data.CommandQueueCopy->ResetCommandLists();

			std::this_thread::yield();
		}
		});
}

void RenderBackend::BeginFrame()
{
	SCOPED_TIMER("RenderBackend::BeginFrame");

	s_Data.TimestampQueries[s_Data.CurrentBackBufferIndex].clear();
	s_Data.NextQueryIndex[s_Data.CurrentBackBufferIndex] = 0;
}

void RenderBackend::OnImGuiRender()
{
	ImGui::Text("DX12 render backend");
	ImGui::Text("API calls (WIP)");
}

void RenderBackend::EndFrame()
{
	SCOPED_TIMER("RenderBackend::EndFrame");

	s_Data.SwapChain->SwapBuffers(s_Data.VSync);
	s_Data.CurrentBackBufferIndex = s_Data.SwapChain->GetCurrentBackBufferIndex();

	ProcessTimestampQueries();
}

void RenderBackend::Finalize()
{
	Flush();

	s_Data.ProcessInFlightCommandLists = false;

	if (s_Data.ProcessInFlightCommandListsThread.joinable())
		s_Data.ProcessInFlightCommandListsThread.join();
}

void RenderBackend::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	auto commandList = s_Data.CommandQueueCopy->GetCommandList();
	commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);
	uint64_t fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	auto commandList = s_Data.CommandQueueCopy->GetCommandList();
	commandList->CopyBufferRegion(intermediateBuffer, intermediateOffset, destBuffer, destOffset, numBytes);
	uint64_t fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	auto commandList = s_Data.CommandQueueCopy->GetCommandList();
	commandList->CopyTexture(intermediateBuffer, destTexture, textureData);
	uint64_t fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
}

ID3D12QueryHeap* RenderBackend::GetD3D12TimestampQueryHeap()
{
	return s_Data.D3D12QueryHeapTimestamp.Get();
}

const Buffer& RenderBackend::GetQueryResultBuffer(uint32_t backBufferIndex)
{
	return *s_Data.QueryResultBuffers[backBufferIndex];
}

void RenderBackend::TrackTimestampQueryResult(const std::string& name, const TimestampQuery& timestampQuery, uint32_t backBufferIndex)
{
	s_Data.TimestampQueries[backBufferIndex].emplace(name, timestampQuery);
	ASSERT(s_Data.TimestampQueries[backBufferIndex].size() <= s_Data.MaxQueriesPerFrame / 2, "Exceeded the maximum amount of queries per frame");
}

uint32_t RenderBackend::GetMaxQueriesPerFrame()
{
	return s_Data.MaxQueriesPerFrame;
}

uint32_t RenderBackend::GetNextQueryIndex(uint32_t backBufferIndex)
{
	return s_Data.NextQueryIndex[backBufferIndex]++;
}

void RenderBackend::Resize(uint32_t width, uint32_t height)
{
	width = std::max(1u, width);
	height = std::max(1u, height);

	Flush();

	s_Data.SwapChain->Resize(width, height);
}

void RenderBackend::Flush()
{
	s_Data.CommandQueueDirect->Flush();
	s_Data.CommandQueueCompute->Flush();
	s_Data.CommandQueueCopy->Flush();
}

void RenderBackend::SetVSync(bool vSync)
{
	s_Data.VSync = vSync;
}

std::shared_ptr<Device> RenderBackend::GetDevice()
{
	return s_Data.Device;
}

SwapChain& RenderBackend::GetSwapChain()
{
	return *s_Data.SwapChain.get();
}

DescriptorAllocation RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return s_Data.DescriptorHeaps[type]->Allocate(numDescriptors);
}

DescriptorHeap& RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return *s_Data.DescriptorHeaps[type].get();
}

std::shared_ptr<CommandList> RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return s_Data.CommandQueueDirect->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return s_Data.CommandQueueCompute->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return s_Data.CommandQueueCopy->GetCommandList();
	}

	ASSERT(false, "Tried to retrieve command list from a command queue type that is not supported.");
	return nullptr;
}

void RenderBackend::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		s_Data.CommandQueueDirect->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		s_Data.CommandQueueCompute->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}

void RenderBackend::ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList)
{
	uint64_t fenceValue = 0;

	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		fenceValue = s_Data.CommandQueueDirect->ExecuteCommandList(commandList);
		s_Data.CommandQueueDirect->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		fenceValue = s_Data.CommandQueueCompute->ExecuteCommandList(commandList);
		s_Data.CommandQueueCompute->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
		s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}

void RenderBackend::ProcessTimestampQueries()
{
	uint32_t nextFrameBufferIndex = (s_Data.CurrentBackBufferIndex + 1) % s_Data.SwapChain->GetBackBufferCount();

	for (auto& [name, timestampQuery] : s_Data.TimestampQueries[nextFrameBufferIndex])
	{
		timestampQuery.ReadFromBuffer(*s_Data.QueryResultBuffers[nextFrameBufferIndex]);

		TimerResult timer = {};
		timer.Name = name.c_str();
		timer.Duration = (timestampQuery.EndQueryTimestamp - timestampQuery.BeginQueryTimestamp) / (double)s_Data.CommandQueueDirect->GetTimestampFrequency() * 1000.0f;

		Profiler::AddGPUTimer(timer);
	}
}
