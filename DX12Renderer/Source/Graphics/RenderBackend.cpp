#include "Pch.h"
#include "Graphics/RenderBackend.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"

static RenderBackend* s_Instance = nullptr;

void RenderBackend::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
	if (!s_Instance)
		s_Instance = new RenderBackend();

	s_Instance->m_Device = std::make_shared<Device>();
	
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		s_Instance->m_DescriptorHeaps[i] = std::make_shared<DescriptorHeap>(s_Instance->m_Device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

	s_Instance->m_CommandQueueDirect = std::make_shared<CommandQueue>(s_Instance->m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	s_Instance->m_CommandQueueCompute = std::make_unique<CommandQueue>(s_Instance->m_Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	s_Instance->m_CommandQueueCopy = std::make_unique<CommandQueue>(s_Instance->m_Device, D3D12_COMMAND_LIST_TYPE_COPY);

	s_Instance->m_SwapChain = std::make_shared<SwapChain>(hWnd, s_Instance->m_CommandQueueDirect, width, height);

	s_Instance->m_ProcessInFlightCommandLists = true;
	s_Instance->m_ProcessInFlightCommandListsThread = std::thread([]() {
		while (s_Instance->m_ProcessInFlightCommandLists)
		{
			s_Instance->m_CommandQueueDirect->ResetCommandLists();
			s_Instance->m_CommandQueueCompute->ResetCommandLists();
			s_Instance->m_CommandQueueCopy->ResetCommandLists();

			std::this_thread::yield();
		}
		});
}

void RenderBackend::Finalize()
{
	Flush();

	s_Instance->m_ProcessInFlightCommandLists = false;

	if (s_Instance->m_ProcessInFlightCommandListsThread.joinable())
		s_Instance->m_ProcessInFlightCommandListsThread.join();
}

void RenderBackend::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	auto commandList = s_Instance->m_CommandQueueCopy->GetCommandList();
	commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);
	uint64_t fenceValue = s_Instance->m_CommandQueueCopy->ExecuteCommandList(commandList);
	s_Instance->m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	auto commandList = s_Instance->m_CommandQueueCopy->GetCommandList();
	commandList->CopyBufferRegion(intermediateBuffer, intermediateOffset, destBuffer, destOffset, numBytes);
	uint64_t fenceValue = s_Instance->m_CommandQueueCopy->ExecuteCommandList(commandList);
	s_Instance->m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	auto commandList = s_Instance->m_CommandQueueCopy->GetCommandList();
	commandList->CopyTexture(intermediateBuffer, destTexture, textureData);
	uint64_t fenceValue = s_Instance->m_CommandQueueCopy->ExecuteCommandList(commandList);
	s_Instance->m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::Resize(uint32_t width, uint32_t height)
{
	width = std::max(1u, width);
	height = std::max(1u, height);

	Flush();

	s_Instance->m_SwapChain->Resize(width, height);
}

void RenderBackend::Flush()
{
	s_Instance->m_CommandQueueDirect->Flush();
	s_Instance->m_CommandQueueCompute->Flush();
	s_Instance->m_CommandQueueCopy->Flush();
}

std::shared_ptr<Device> RenderBackend::GetDevice()
{
	return s_Instance->m_Device;
}

std::shared_ptr<SwapChain> RenderBackend::GetSwapChain()
{
	return s_Instance->m_SwapChain;
}

DescriptorAllocation RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return s_Instance->m_DescriptorHeaps[type]->Allocate(numDescriptors);
}

std::shared_ptr<DescriptorHeap> RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return s_Instance->m_DescriptorHeaps[type];
}

std::shared_ptr<CommandList> RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return s_Instance->m_CommandQueueDirect->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return s_Instance->m_CommandQueueCompute->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return s_Instance->m_CommandQueueCopy->GetCommandList();
	}

	ASSERT(false, "Tried to retrieve command list from a command queue type that is not supported.");
	return nullptr;
}

void RenderBackend::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		s_Instance->m_CommandQueueDirect->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		s_Instance->m_CommandQueueCompute->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		s_Instance->m_CommandQueueCopy->ExecuteCommandList(commandList);
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
		fenceValue = s_Instance->m_CommandQueueDirect->ExecuteCommandList(commandList);
		s_Instance->m_CommandQueueDirect->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		fenceValue = s_Instance->m_CommandQueueCompute->ExecuteCommandList(commandList);
		s_Instance->m_CommandQueueCompute->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		fenceValue = s_Instance->m_CommandQueueCopy->ExecuteCommandList(commandList);
		s_Instance->m_CommandQueueCopy->WaitForFenceValue(fenceValue);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}
