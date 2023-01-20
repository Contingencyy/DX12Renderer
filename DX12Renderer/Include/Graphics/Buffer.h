#pragma once
#include "Graphics/Resource.h"
#include "Graphics/RenderAPI.h"

class Buffer : public Resource
{
public:
	Buffer(const BufferDesc& params);
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
