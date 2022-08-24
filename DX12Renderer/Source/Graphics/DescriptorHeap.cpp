#include "Pch.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/Device.h"

DescriptorHeap::DescriptorHeap(std::shared_ptr<Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    : m_NumDescriptors(numDescriptors), m_Device(device)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = numDescriptors;
    heapDesc.Type = type;
    heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    m_Device->CreateDescriptorHeap(heapDesc, m_d3d12DescriptorHeap);
    m_DescriptorHandleIncrementSize = m_Device->GetDescriptorIncrementSize(type);

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
