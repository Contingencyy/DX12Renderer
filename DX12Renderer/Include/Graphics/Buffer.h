#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"

enum class BufferUsage : uint32_t
{
	BUFFER_USAGE_NONE = 0,
	BUFFER_USAGE_VERTEX = (1 << 0),
	BUFFER_USAGE_INDEX = (1 << 1),
	BUFFER_USAGE_READ = (1 << 2),
	BUFFER_USAGE_WRITE = (1 << 3),
	BUFFER_USAGE_CONSTANT = (1 << 4),
	BUFFER_USAGE_UPLOAD = (1 << 5)
};

inline bool operator&(BufferUsage lhs, BufferUsage rhs)
{
	return static_cast<uint32_t>(lhs) & static_cast<uint32_t>(rhs);
}

inline BufferUsage operator|(BufferUsage lhs, BufferUsage rhs)
{
	return static_cast<BufferUsage>(static_cast<uint32_t>(lhs) | static_cast<uint32_t>(rhs));
}

struct BufferDesc
{
	BufferDesc() = default;
	BufferDesc(BufferUsage usage, std::size_t numElements, std::size_t elementSize)
		: Usage(usage), NumElements(numElements), ElementSize(elementSize) {}

	BufferUsage Usage = BufferUsage::BUFFER_USAGE_NONE;

	std::size_t NumElements = 0;
	std::size_t ElementSize = 0;
};

D3D12_RESOURCE_STATES BufferUsageToD3DResourceState(BufferUsage usage);

class Buffer
{
public:
	Buffer(const std::string& name, const BufferDesc& bufferdesc, const void* data);
	Buffer(const std::string& name, const BufferDesc& bufferDesc);
	~Buffer();

	// This should not be called every frame, since it allocates another heap/buffer for upload in this function
	// Might be a good idea to have a larger upload heap on the command list to suballocate from for copying/staging.
	void SetBufferData(const void* data, std::size_t byteSize = 0);
	void SetBufferDataAtOffset(const void* data, std::size_t byteSize, std::size_t byteOffset);
	bool IsValid() const;

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(DescriptorType type) const;
	uint32_t GetDescriptorIndex(DescriptorType type) const;

	BufferDesc GetBufferDesc() const { return m_BufferDesc; }
	std::size_t GetByteSize() const { return m_ByteSize; }
	std::string GetName() const { return m_Name; }
	void SetName(const std::string& name);
	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();
	void CreateViews();

private:
	BufferDesc m_BufferDesc = {};
	DescriptorAllocation m_DescriptorAllocations[DescriptorType::NUM_DESCRIPTOR_TYPES] = {};

	std::size_t m_ByteSize = 0;
	std::string m_Name = "";

	ComPtr<ID3D12Resource> m_d3d12Resource;
	void* m_CPUPtr = nullptr;

};
