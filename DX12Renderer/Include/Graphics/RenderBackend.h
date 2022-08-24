#pragma once
#include "Graphics/DescriptorAllocation.h"

class Device;
class SwapChain;
class DescriptorHeap;
class CommandQueue;
class CommandList;
class Texture;
class Buffer;

class RenderBackend
{
public:
	static RenderBackend& Get();

	void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	void Finalize();

	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);

	void Resize(uint32_t width, uint32_t height);
	void Flush();

	std::shared_ptr<Device> GetDevice() const { return m_Device; }

	void ResolveToBackBuffer(const Texture& texture);
	void SwapBuffers(bool vSync);
	DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);

	std::shared_ptr<CommandList> GetCommandList(D3D12_COMMAND_LIST_TYPE type);
	void ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	void ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList);
	
private:
	RenderBackend();
	~RenderBackend();

private:
	std::shared_ptr<Device> m_Device;
	std::unique_ptr<SwapChain> m_SwapChain;
	std::unique_ptr<DescriptorHeap> m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::unique_ptr<CommandQueue> m_CommandQueueCompute;
	std::unique_ptr<CommandQueue> m_CommandQueueCopy;

	std::thread m_ProcessInFlightCommandListsThread;
	std::atomic_bool m_ProcessInFlightCommandLists;

};
