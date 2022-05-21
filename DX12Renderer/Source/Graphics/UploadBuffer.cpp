#include "Pch.h"
#include "Graphics/UploadBuffer.h"
#include "Application.h"
#include "Renderer.h"

UploadBuffer::UploadBuffer(std::size_t size)
	: Buffer()
{
    Application::Get().GetRenderer()->CreateBuffer(m_d3d12Resource, D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ, size);

    if (m_d3d12Resource)
    {
        m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
        m_AlignedBufferSize = size;
    }
}

UploadBuffer::~UploadBuffer()
{
    m_d3d12Resource->Unmap(0, nullptr);
    m_CPUPtr = nullptr;
}

void UploadBuffer::CreateView()
{
}   
