#include "Pch.h"
#include "Graphics/DescriptorAllocation.h"
#include "Graphics/DescriptorHeap.h"

DescriptorAllocation::DescriptorAllocation()
	: m_DescriptorHandle(CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT)), m_NumDescriptors(0), m_DescriptorHandleIncrementSize(0)
{
}

DescriptorAllocation::DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numDescriptors, uint32_t descriptorSize)
	: m_DescriptorHandle(descriptor), m_NumDescriptors(numDescriptors), m_DescriptorHandleIncrementSize(descriptorSize)
{
}

DescriptorAllocation::~DescriptorAllocation()
{
	Free();
}

D3D12_CPU_DESCRIPTOR_HANDLE DescriptorAllocation::GetDescriptorHandle(uint32_t offset)
{
	ASSERT((offset < m_NumDescriptors), "Offset is bigger than the total number of descriptors in descriptor allocation");
	return { m_DescriptorHandle.ptr + (m_DescriptorHandleIncrementSize * offset) };
}

bool DescriptorAllocation::IsNull()
{
	return m_DescriptorHandle.ptr == 0;
}

void DescriptorAllocation::Free()
{
	if (!IsNull())
	{
		m_DescriptorHandle.ptr = 0;
		m_NumDescriptors = 0;
		m_DescriptorHandleIncrementSize = 0;
	}
}
