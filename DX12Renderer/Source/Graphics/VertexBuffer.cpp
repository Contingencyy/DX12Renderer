#include "Pch.h"
#include "Graphics/VertexBuffer.h"

VertexBuffer::VertexBuffer()
	: Buffer()
{
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateView(std::size_t numVertices, std::size_t vertexSize)
{
	m_NumVertices = numVertices;
	m_VertexStride = vertexSize;

	m_VertexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = static_cast<uint32_t>(m_NumVertices * m_VertexStride);
	m_VertexBufferView.StrideInBytes = static_cast<uint32_t>(m_VertexStride);
}

D3D12_VERTEX_BUFFER_VIEW VertexBuffer::GetView() const
{
	return m_VertexBufferView;
}

std::size_t VertexBuffer::GetNumVertices() const
{
	return m_NumVertices;
}

std::size_t VertexBuffer::GetVertexStride() const
{
	return m_VertexStride;
}
