#include "Pch.h"
#include "Graphics/VertexBuffer.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/UploadBuffer.h"

VertexBuffer::VertexBuffer(std::size_t numVertices, std::size_t vertexSize)
	: Buffer(), m_NumVertices(numVertices), m_VertexStride(vertexSize)
{
	m_AlignedBufferSize = MathHelper::AlignUp(m_NumVertices * m_VertexStride, m_VertexStride);
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateView()
{
	m_VertexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = static_cast<uint32_t>(m_AlignedBufferSize);
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
