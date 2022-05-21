#pragma once
#include "Graphics/Buffer.h"

class UploadBuffer : public Buffer
{
public:
	UploadBuffer(std::size_t size);
	virtual ~UploadBuffer();

	virtual void CreateView();

private:
	void* m_CPUPtr = nullptr;

};
