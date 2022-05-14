#pragma once
#include "Graphics/Buffer.h"

class UploadBuffer : public Buffer
{
public:
	explicit UploadBuffer(std::size_t size);
	virtual ~UploadBuffer();

	virtual void CreateView(std::size_t numElements, std::size_t elementSize);

	void UploadDataToBuffer(Buffer& buffer, std::size_t numElements, std::size_t elementSize, const void* data);

private:
	void* m_CPUPtr = nullptr;
	std::size_t m_Size = 0;

};
