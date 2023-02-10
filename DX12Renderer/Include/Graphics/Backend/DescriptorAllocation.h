#pragma once

class DescriptorHeap;

enum DescriptorType : uint32_t
{
	CBV, SRV, UAV, RTV, DSV, NUM_DESCRIPTOR_TYPES
};

class DescriptorAllocation
{
public:
	DescriptorAllocation();
	DescriptorAllocation(D3D12_CPU_DESCRIPTOR_HANDLE cpu, D3D12_GPU_DESCRIPTOR_HANDLE gpu, uint32_t offset, uint32_t numDescriptors, uint32_t descriptorSize);
	~DescriptorAllocation();

	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(uint32_t offset = 0) const;
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(uint32_t offset = 0) const;
	uint32_t GetOffsetInDescriptorHeap() const { return m_OffsetInDescriptorHeap; }

	bool IsNull();

private:
	void Free();

private:
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUDescriptorHandle;
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUDescriptorHandle;

	uint32_t m_OffsetInDescriptorHeap;
	uint32_t m_NumDescriptors;
	uint32_t m_DescriptorHandleIncrementSize;

};
