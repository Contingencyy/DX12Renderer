#include "Pch.h"
#include "Graphics/Backend/DescriptorAllocation.h"
#include "Graphics/Backend/DescriptorHeap.h"

DescriptorAllocation::DescriptorAllocation()
	: m_CPUDescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)), m_OffsetInDescriptorHeap(0), m_NumDescriptors(0), m_DescriptorHandleIncrementSize(0)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t offset, uint32_t numDescriptors, uint32_t descriptorSize)
	: m_CPUDescriptorHandle(descriptor), m_OffsetInDescriptorHeap(offset), m_NumDescriptors(numDescriptors), m_DescriptorHandleIncrementSize(descriptorSize)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetCPUDescriptorHandle(uint32_t offset) const
{
	ASSERT(offset < m_NumDescriptors, "Offset is bigger than the total number of descriptors in descriptor allocation");
	return { m_CPUDescriptorHandle.ptr + (m_DescriptorHandleIncrementSize * offset) };
}

bool DescriptorAllocation::IsNull()
{
	return m_CPUDescriptorHandle.ptr == 0;
}

void DescriptorAllocation::Free()
{
	if (!IsNull())
	{
		m_CPUDescriptorHandle.ptr = 0;
		m_OffsetInDescriptorHeap = 0;
		m_NumDescriptors = 0;
		m_DescriptorHandleIncrementSize = 0;
	}
}
