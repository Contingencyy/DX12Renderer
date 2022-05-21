#pragma once
#include "Graphics/Buffer.h"

class IndexBuffer : public Buffer
{
public:
	IndexBuffer(std::size_t numIndices, std::size_t indexSize);
	virtual ~IndexBuffer();

	virtual void CreateView();

	D3D12_INDEX_BUFFER_VIEW GetView() const;
	std::size_t GetNumIndices() const;
	std::size_t GetIndexSize() const;

private:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	std::size_t m_NumIndices = 0;
	std::size_t m_IndexSize = 0;

};
