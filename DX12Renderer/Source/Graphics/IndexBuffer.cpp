#include "Pch.h"
#include "Graphics/IndexBuffer.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/UploadBuffer.h"

IndexBuffer::IndexBuffer(std::size_t numIndices, std::size_t indexSize)
	: Buffer(), m_NumIndices(numIndices), m_IndexSize(indexSize)
{
	ASSERT((indexSize == 2 || indexSize == 4), "Index has to be either a 16-bit or a 32-bit unsigned integer.");
	m_AlignedBufferSize = MathHelper::AlignUp(numIndices * indexSize, indexSize);
}

IndexBuffer::~IndexBuffer()
{
}

void IndexBuffer::CreateView()
{
	m_IndexBufferView.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
	m_IndexBufferView.SizeInBytes = static_cast<uint32_t>(m_AlignedBufferSize);
	m_IndexBufferView.Format = (m_IndexSize == 2) ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;
}

D3D12_INDEX_BUFFER_VIEW IndexBuffer::GetView() const
{
	return m_IndexBufferView;
}

std::size_t IndexBuffer::GetNumIndices() const
{
	return m_NumIndices;
}

std::size_t IndexBuffer::GetIndexSize() const
{
	return m_IndexSize;
}
