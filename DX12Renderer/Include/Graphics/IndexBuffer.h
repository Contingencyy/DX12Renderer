#pragma once
#include "Graphics/Buffer.h"

class IndexBuffer : public Buffer
{
public:
	explicit IndexBuffer();
	virtual ~IndexBuffer();

	virtual void CreateView(std::size_t numIndices, std::size_t indexSize);

	D3D12_INDEX_BUFFER_VIEW GetView() const;
	std::size_t GetNumIndices() const;
	DXGI_FORMAT GetIndexFormat() const;

private:
	D3D12_INDEX_BUFFER_VIEW m_IndexBufferView = {};

	std::size_t m_NumIndices;
	DXGI_FORMAT m_IndexFormat;

};
