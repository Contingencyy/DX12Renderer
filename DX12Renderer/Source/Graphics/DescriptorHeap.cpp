#include "Pch.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/Device.h"

DescriptorHeap::DescriptorHeap(std::shared_ptr<Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
    : m_Device(device), m_Type(type), m_NumDescriptors(numDescriptors)
{
    D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
    heapDesc.NumDescriptors = m_NumDescriptors;
    heapDesc.Type = type;

    // Set the flag to shader visible if the descriptor heap is of type CBV_SRV_UAV for bindless rendering
    heapDesc.Flags = type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE : D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

    m_Device->CreateDescriptorHeap(heapDesc, m_d3d12DescriptorHeap);
    m_DescriptorHandleIncrementSize = m_Device->GetDescriptorIncrementSize(type);

    m_CPUBaseDescriptor = m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();

    if (heapDesc.Flags == D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE)
        m_GPUBaseDescriptor = m_d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart();
}

DescriptorHeap::~DescriptorHeap()
{
}

DescriptorAllocation DescriptorHeap::Allocate(uint32_t numDescriptors)
{
    ASSERT(m_DescriptorOffset + numDescriptors < m_NumDescriptors, "Failed to satisfy descriptor allocation request, descriptor heap is too small");

    DescriptorAllocation allocation(CD3DX12_CPU_DESCRIPTOR_HANDLE(m_CPUBaseDescriptor, m_DescriptorOffset, m_DescriptorHandleIncrementSize),
        m_DescriptorOffset, numDescriptors, m_DescriptorHandleIncrementSize);
    m_DescriptorOffset += numDescriptors;

    return allocation;
}

void DescriptorHeap::Reset()
{
    m_DescriptorOffset = 0;
}
