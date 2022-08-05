#pragma once
#include "Graphics/CommandList.h"
#include "Graphics/RootSignature.h"

class DynamicDescriptorHeap
{
public:
	DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors = 512);
	~DynamicDescriptorHeap();

	void ParseRootSignature(const RootSignature& rootSignature);
	void StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor);
	void CommitStagedDescriptors(CommandList& commandList);

	void Reset();

private:
	static const uint32_t s_MaxDescriptorTables = 32;

	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_TYPE m_Type;

	CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescriptorHandle;
	CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescriptorHandle;

	struct DescriptorTableCache
	{
		void Reset()
		{
			NumDescriptors = 0;
			BaseDescriptor = nullptr;
		}

		uint32_t NumDescriptors = 0;
		D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor = nullptr;
	};

	std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache;
	std::unordered_map<uint32_t, DescriptorTableCache> m_DescriptorTableCache;

	uint32_t m_NumDescriptors = 0;
	uint32_t m_DescriptorHandleIncrementSize = 0;
	uint32_t m_NumDescriptorsToCommit = 0;

};
