#include "Pch.h"
#include "Graphics/Backend/UploadBuffer.h"
#include "Graphics/Backend/RenderBackend.h"

UploadBuffer::UploadBuffer()
{
	m_TotalByteSize = MEGABYTE(500);

	CD3DX12_RESOURCE_DESC desc = CD3DX12_RESOURCE_DESC::Buffer(m_TotalByteSize);
	RenderBackend::CreateBuffer(m_d3d12Resource, D3D12_HEAP_TYPE_UPLOAD, desc, D3D12_RESOURCE_STATE_GENERIC_READ);

	m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
}

UploadBuffer::~UploadBuffer()
{
	m_d3d12Resource->Unmap(0, nullptr);
}

UploadBufferAllocation UploadBuffer::Allocate(std::size_t byteSize, std::size_t align)
{
	std::size_t alignedByteSize = MathHelper::AlignUp(byteSize, align);
	ASSERT(m_CurrentByteOffset + alignedByteSize < m_TotalByteSize, "Upload buffer could not satisfy the allocation request");

	UploadBufferAllocation alloc = {};
	alloc.D3D12Resource = m_d3d12Resource.Get();
	alloc.Size = alignedByteSize;
	alloc.OffsetInBuffer = m_CurrentByteOffset;

	m_CurrentByteOffset += alloc.Size;
	return alloc;
}
