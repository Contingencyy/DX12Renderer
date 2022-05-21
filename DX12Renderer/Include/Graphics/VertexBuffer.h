#pragma once
#include "Graphics/Buffer.h"

class VertexBuffer : public Buffer
{
public:
	VertexBuffer(std::size_t numVertices, std::size_t vertexSize);
	virtual ~VertexBuffer();

	virtual void CreateView();

	D3D12_VERTEX_BUFFER_VIEW GetView() const;
	std::size_t GetNumVertices() const;
	std::size_t GetVertexStride() const;

private:
	D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView = {};

	std::size_t m_NumVertices = 0;
	std::size_t m_VertexStride = 0;

};
