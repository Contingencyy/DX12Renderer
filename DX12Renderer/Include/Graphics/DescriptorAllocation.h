#pragma once

class DescriptorHeap;

class DescriptorAllocation
{
public:
	DescriptorAllocation();
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE descriptor, uint32_t numDescriptors, uint32_t descriptorSize);
	~DescriptorAllocation();

	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptorHandle(uint32_t offset = 0);
	bool IsNull();

private:
	void Free();

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_DescriptorHandle;

	uint32_t m_NumDescriptors;
	uint32_t m_DescriptorHandleIncrementSize;

};
