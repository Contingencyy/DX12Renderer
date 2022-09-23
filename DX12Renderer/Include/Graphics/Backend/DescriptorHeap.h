#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"

class Device;

class DescriptorHeap
{
public:
	DescriptorHeap(std::shared_ptr<Device> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 256);
	~DescriptorHeap();

	DescriptorAllocation Allocate(uint32_t numDescriptors = 1);
	void Reset();

	uint32_t GetNumDescriptors() const { return m_NumDescriptors; }
	uint32_t GetDescriptorHandleIncrementSize() const { return m_DescriptorHandleIncrementSize; }
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUBaseDescriptor() const { return m_CPUBaseDescriptor; }
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUBaseDescriptor() const { return m_GPUBaseDescriptor; }

	ComPtr<ID3D12DescriptorHeap> GetD3D12DescriptorHeap() const { return m_d3d12DescriptorHeap; }

private:
	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;
	
	D3D12_CPU_DESCRIPTOR_HANDLE m_CPUBaseDescriptor = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	D3D12_GPU_DESCRIPTOR_HANDLE m_GPUBaseDescriptor = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);

	std::shared_ptr<Device> m_Device;

	uint32_t m_NumDescriptors = 256;
	uint32_t m_DescriptorOffset = 0;
	uint32_t m_DescriptorHandleIncrementSize = 0;

};
