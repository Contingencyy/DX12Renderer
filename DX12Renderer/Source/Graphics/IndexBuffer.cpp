#include "Pch.h"
#include "Graphics/IndexBuffer.h"

IndexBuffer::IndexBuffer()
	: Buffer()
{
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::CreateView(std::size_t numIndices, std::size_t indexSize)
{
	ASSERT((indexSize == 2 || indexSize == 4), "Index has to be either a 16-bit or a 32-bit unsigned integer.");

	m_NumIndices = numIndices;
	m_IndexFormat = (indexSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_IndexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes = static_cast<uint32_t>(m_NumIndices * indexSize);
	m_IndexBufferView.Format = m_IndexFormat;
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const
{
	return m_IndexBufferView;
}

std::size_t IndexBuffer::GetNumIndices() const
{
	return m_NumIndices;
}

DXGI_FORMAT IndexBuffer::GetIndexFormat() const
{
	return m_IndexFormat;
}
