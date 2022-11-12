#include "Pch.h"
#include "Graphics/Resource.h"

Resource::Resource(const std::string& name)
{
}

Resource::~Resource()
{
}

const DescriptorAllocation& Resource::GetDescriptorAllocation(DescriptorType type) const
{
	return m_DescriptorAllocations[type];
}

D3D12_CPU_DESCRIPTOR_HANDLE Resource::GetDescriptor(DescriptorType type, uint32_t offset) const
{
	return m_DescriptorAllocations[type].GetCPUDescriptorHandle(offset);
}

uint32_t Resource::GetDescriptorHeapIndex(DescriptorType type, uint32_t offset) const
{
	return m_DescriptorAllocations[type].GetOffsetInDescriptorHeap() + offset;
}

void Resource::SetName(const std::string& name)
{
	m_Name = name;
	m_d3d12Resource->SetName(StringHelper::StringToWString(name).c_str());
}

void Resource::ResetDescriptorAllocations()
{
	for (uint32_t i = 0; i < DescriptorType::NUM_DESCRIPTOR_TYPES; ++i)
	{
		m_DescriptorAllocations[i].~DescriptorAllocation();
	}
}
