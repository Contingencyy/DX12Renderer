#include "Pch.h"
#include "Graphics/DescriptorHeap.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

DescriptorHeap::DescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    : m_NumDescriptors(numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    auto device = Application::Get().GetRenderer()->GetDevice();

    device->CreateDescriptorHeap(heapDesc, m_d3d12DescriptorHeap);
    m_DescriptorHandleIncrementSize = device->GetDescriptorIncrementSize(type);

    m_BaseDescriptor = m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
}

DescriptorHeap::~DescriptorHeap()
{
}

DescriptorAllocation DescriptorHeap::Allocate(uint32_t numDescriptors)
{
    ASSERT((m_DescriptorOffset + numDescriptors < m_NumDescriptors), "Failed to satisfy descriptor allocation request, descriptor heap is too small");

    DescriptorAllocation allocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, m_DescriptorOffset, m_DescriptorHandleIncrementSize), numDescriptors, m_DescriptorHandleIncrementSize);
    m_DescriptorOffset += numDescriptors;

    return allocation;
}

void DescriptorHeap::Reset()
{
    m_DescriptorOffset = 0;
}
