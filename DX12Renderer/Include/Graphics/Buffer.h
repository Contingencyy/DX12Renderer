#pragma once
#include "Graphics/Resource.h"

enum class BufferUsage : uint32_t
{
	BUFFER_USAGE_NONE = 0,
	BUFFER_USAGE_VERTEX = (1 << 0),
	BUFFER_USAGE_INDEX = (1 << 1),
	BUFFER_USAGE_READ = (1 << 2),
	BUFFER_USAGE_WRITE = (1 << 3),
	BUFFER_USAGE_CONSTANT = (1 << 4),
	BUFFER_USAGE_UPLOAD = (1 << 5),
	BUFFER_USAGE_READBACK = (1 << 6),
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
	BufferDesc(BufferUsage usage, std::size_t byteSize)
		: Usage(usage), NumElements(1), ElementSize(byteSize) {}

	BufferUsage Usage = BufferUsage::BUFFER_USAGE_NONE;

	std::size_t NumElements = 0;
	std::size_t ElementSize = 0;
};

class Buffer : public Resource
{
public:
	Buffer(const std::string& name, const BufferDesc& bufferDesc);
	Buffer(const std::string& name, const BufferDesc& bufferdesc, const void* data);
	virtual ~Buffer();

	virtual bool IsValid() const;
	virtual bool IsCPUAccessible() const;
	virtual void Invalidate();

	void SetBufferData(const void* data, std::size_t byteSize = 0);
	void SetBufferDataAtOffset(const void* data, std::size_t byteSize, std::size_t byteOffset);

	template<typename T>
	T ReadBackBytes(std::size_t numBytes, std::size_t byteOffset) const
	{
		T data = { 0 };
		memcpy(&data, static_cast<unsigned char*>(m_CPUPtr) + byteOffset, numBytes);
		return data;
	}

	BufferDesc& GetBufferDesc() { return m_BufferDesc; }
	const BufferDesc& GetBufferDesc() const { return m_BufferDesc; }

protected:
	virtual void CreateD3D12Resource();
	virtual void AllocateDescriptors();
	virtual void CreateViews();

protected:
	BufferDesc m_BufferDesc = {};

};
