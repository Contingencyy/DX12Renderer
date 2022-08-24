#include "Pch.h"
#include "Graphics/RenderBackend.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/CommandQueue.h"

static RenderBackend* s_Instance = nullptr;

RenderBackend& RenderBackend::Get()
{
	if (!s_Instance)
	{
		s_Instance = new RenderBackend();
	}

	return *s_Instance;
}

void RenderBackend::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
	if (!s_Instance)
	{
		s_Instance = new RenderBackend();
	}

	m_Device = std::make_shared<Device>();
	
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		m_DescriptorHeaps[i] = std::make_unique<DescriptorHeap>(m_Device, static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

	m_CommandQueueDirect = std::make_shared<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	m_CommandQueueCompute = std::make_unique<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_COMPUTE);
	m_CommandQueueCopy = std::make_unique<CommandQueue>(m_Device, D3D12_COMMAND_LIST_TYPE_COPY);

	m_SwapChain = std::make_unique<SwapChain>(hWnd, m_CommandQueueDirect, width, height);

	m_ProcessInFlightCommandLists = true;
	m_ProcessInFlightCommandListsThread = std::thread([this]() {
		while (m_ProcessInFlightCommandLists)
		{
			m_CommandQueueDirect->ResetCommandLists();
			m_CommandQueueCompute->ResetCommandLists();
			m_CommandQueueCopy->ResetCommandLists();

			//std::this_thread::yield();
		}
		});
}

void RenderBackend::Finalize()
{
	Flush();

	m_ProcessInFlightCommandLists = false;

	if (m_ProcessInFlightCommandListsThread.joinable())
		m_ProcessInFlightCommandListsThread.join();
}

void RenderBackend::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	auto commandList = m_CommandQueueCopy->GetCommandList();
	commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);
	uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
	m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	auto commandList = m_CommandQueueCopy->GetCommandList();
	commandList->CopyTexture(intermediateBuffer, destTexture, textureData);
	uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
	m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::Resize(uint32_t width, uint32_t height)
{
	width = std::max(1u, width);
	height = std::max(1u, height);

	Flush();

	m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Reset();
	m_SwapChain->Resize(width, height);
}

void RenderBackend::Flush()
{
	m_CommandQueueDirect->Flush();
	m_CommandQueueCompute->Flush();
	m_CommandQueueCopy->Flush();
}

void RenderBackend::ResolveToBackBuffer(const Texture& texture)
{
	m_SwapChain->ResolveToBackBuffer(texture);
}

void RenderBackend::SwapBuffers(bool vSync)
{
	m_SwapChain->SwapBuffers(vSync);
}

DescriptorAllocation RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return m_DescriptorHeaps[type]->Allocate(numDescriptors);
}

std::shared_ptr<CommandList> RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return m_CommandQueueDirect->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return m_CommandQueueCompute->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return m_CommandQueueCopy->GetCommandList();
	}

	ASSERT(false, "Tried to retrieve command list from a command queue type that is not supported.");
	return nullptr;
}

void RenderBackend::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		m_CommandQueueDirect->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		m_CommandQueueCompute->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		m_CommandQueueCopy->ExecuteCommandList(commandList);
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
		fenceValue = m_CommandQueueDirect->ExecuteCommandList(commandList);
		m_CommandQueueDirect->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		fenceValue = m_CommandQueueCompute->ExecuteCommandList(commandList);
		m_CommandQueueCompute->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
		m_CommandQueueCopy->WaitForFenceValue(fenceValue);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}

RenderBackend::RenderBackend()
{
}

RenderBackend::~RenderBackend()
{
}
