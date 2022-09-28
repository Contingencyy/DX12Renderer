#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"

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
	static void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	static void Finalize();

	static void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	static void CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes);
	static void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);

	static void Resize(uint32_t width, uint32_t height);
	static void Flush();

	static std::shared_ptr<Device> GetDevice();
	static SwapChain& GetSwapChain();

	static DescriptorAllocation AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 1);
	static DescriptorHeap& GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type);

	static std::shared_ptr<CommandList> GetCommandList(D3D12_COMMAND_LIST_TYPE type);
	static void ExecuteCommandList(std::shared_ptr<CommandList> commandList);
	static void ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList);

};
