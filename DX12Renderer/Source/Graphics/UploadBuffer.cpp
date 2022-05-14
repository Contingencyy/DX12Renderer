#include "Pch.h"
#include "Graphics/UploadBuffer.h"
#include "Application.h"
#include "Renderer.h"

UploadBuffer::UploadBuffer(std::size_t size)
	: Buffer()
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_UPLOAD);
    CD3DX12_RESOURCE_DESC heapDesc(CD3DX12_RESOURCE_DESC::Buffer(size));

    HRESULT hr = Application::Get().GetRenderer()->GetD3D12Device()->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &heapDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(&m_d3d12Resource)
    );

    if (SUCCEEDED(hr))
    {
        m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
        m_Size = size;
    }
}

UploadBuffer::~UploadBuffer()
{
    m_d3d12Resource->Unmap(0, nullptr);
    m_CPUPtr = nullptr;
}

void UploadBuffer::CreateView(std::size_t numElements, std::size_t elementSize)
{
}

void UploadBuffer::UploadDataToBuffer(Buffer& buffer, std::size_t numElements, std::size_t elementSize, const void* data)
{
	std::size_t alignedBufferSize = AlignUp(static_cast<std::size_t>(numElements * elementSize), elementSize);
    ComPtr<ID3D12Resource> d3d12DestResource;

    if (alignedBufferSize > 0 && data != nullptr)
    {
        CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
        CD3DX12_RESOURCE_DESC heapDesc(CD3DX12_RESOURCE_DESC::Buffer(alignedBufferSize));

        DX_CALL(Application::Get().GetRenderer()->GetD3D12Device()->CreateCommittedResource(
            &heapProps,
            D3D12_HEAP_FLAG_NONE,
            &heapDesc,
            D3D12_RESOURCE_STATE_COMMON,
            nullptr,
            IID_PPV_ARGS(&d3d12DestResource)
        ));

        D3D12_SUBRESOURCE_DATA subresourceData = {};
        subresourceData.pData = data;
        subresourceData.RowPitch = alignedBufferSize;
        subresourceData.SlicePitch = subresourceData.RowPitch;

        UpdateSubresources(Application::Get().GetRenderer()->GetGraphicsCommandList().Get(),
            d3d12DestResource.Get(), m_d3d12Resource.Get(), 0, 0, 1, &subresourceData);
    }

    buffer.SetD3D12Resource(d3d12DestResource);
    buffer.CreateView(numElements, elementSize);

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
    Application::Get().GetRenderer()->GetGraphicsCommandList()->ResourceBarrier(1, &barrier);
}
