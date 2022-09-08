#pragma once
#include "Graphics/DescriptorAllocation.h"

enum class BufferUsage
{
	BUFFER_USAGE_VERTEX,
	BUFFER_USAGE_INDEX,
	BUFFER_USAGE_CONSTANT,
	BUFFER_USAGE_UPLOAD
};

struct BufferDesc
{
	BufferDesc() = default;
	BufferDesc(BufferUsage usage, std::size_t numElements, std::size_t elementSize)
		: Usage(usage), NumElements(numElements), ElementSize(elementSize) {}

	BufferUsage Usage = BufferUsage::BUFFER_USAGE_VERTEX;

	std::size_t NumElements = 0;
	std::size_t ElementSize = 0;
};

class Buffer
{
public:
	Buffer(const std::string& name, const BufferDesc& bufferdesc, const void* data);
	Buffer(const std::string& name, const BufferDesc& bufferDesc);
	~Buffer();

	// This should not be called every frame, since it allocates another heap/buffer for upload in this function
	// Might be a good idea to have a larger upload heap on the command list to suballocate from for copying/staging.
	void SetBufferData(const void* data, std::size_t byteSize = 0);

	BufferDesc GetBufferDesc() const { return m_BufferDesc; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle();
	std::size_t GetByteSize() const { return m_ByteSize; }

	std::string GetName() const { return m_Name; }
	void SetName(const std::string& name);
	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();

private:
	BufferDesc m_BufferDesc = {};
	DescriptorAllocation m_DescriptorAllocation = {};

	std::size_t m_ByteSize = 0;
	std::string m_Name = "";

	ComPtr<ID3D12Resource> m_d3d12Resource;
	void* m_CPUPtr = nullptr;

};
