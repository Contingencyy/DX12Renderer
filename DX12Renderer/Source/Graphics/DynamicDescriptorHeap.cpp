#include "Pch.h"
#include "Graphics/DynamicDescriptorHeap.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
	: m_Type(type), m_NumDescriptors(numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = m_Type;
	heapDesc.NumDescriptors = m_NumDescriptors;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV || m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
		heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	Application::Get().GetRenderer()->GetDevice()->CreateDescriptorHeap(heapDesc, m_d3d12DescriptorHeap);

	m_CurrentCPUDescriptorHandle = (m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_CurrentGPUDescriptorHandle = (m_d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	m_DescriptorHandleIncrementSize = Application::Get().GetRenderer()->GetDevice()->GetDescriptorIncrementSize(m_Type);
	m_DescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptors);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
}

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
{
	const auto& descriptorTableRanges = m_Type == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ? rootSignature.GetDescriptorTableRanges() : rootSignature.GetSamplerTableRanges();

	uint32_t currentOffset = 0;
	for (auto& range : descriptorTableRanges)
	{
		auto descriptorTableCacheIter = m_DescriptorTableCache.find(range.RootParameterIndex);
		if (descriptorTableCacheIter == m_DescriptorTableCache.end())
		{
			m_DescriptorTableCache.emplace(std::pair<uint32_t, DescriptorTableCache>(range.RootParameterIndex, DescriptorTableCache()));
		}

		DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache.at(range.RootParameterIndex);
		descriptorTableCache.NumDescriptors += range.NumDescriptors;
		descriptorTableCache.BaseDescriptor = m_DescriptorHandleCache.get() + currentOffset;

		currentOffset += range.NumDescriptors;
	}

	ASSERT((currentOffset <= m_NumDescriptors), "The root signature requires more than the maximum number of descriptors per descriptor heap");
}

void DynamicDescriptorHeap::StageDescriptors(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors, const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
	ASSERT((numDescriptors <= m_NumDescriptors), "Descriptors for staging exceed the number of descriptors in descriptor heap");
	ASSERT((rootParameterIndex < s_MaxDescriptorTables), "The specified root parameter is bigger than the maximum number of allowed descriptor tables");

	DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootParameterIndex];

	ASSERT((offset + numDescriptors <= descriptorTableCache.NumDescriptors), "Descriptors for staging exceed the number of descriptors in the table");

	D3D12_CPU_DESCRIPTOR_HANDLE* destDescriptor = (descriptorTableCache.BaseDescriptor + offset);
	for (uint32_t i = 0; i < numDescriptors; ++i)
		destDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, m_DescriptorHandleIncrementSize);

	m_NumDescriptorsToCommit += numDescriptors;
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList)
{
	if (m_NumDescriptorsToCommit > 0)
	{
		auto d3d12CommandList = commandList.GetGraphicsCommandList();
		ASSERT((d3d12CommandList != nullptr), "Graphics command list is empty");

		commandList.SetDescriptorHeap(m_Type, m_d3d12DescriptorHeap.Get());

		for (auto& descriptorTableCache : m_DescriptorTableCache)
		{
			uint32_t numSrcDescriptors = descriptorTableCache.second.NumDescriptors;
			D3D12_CPU_DESCRIPTOR_HANDLE* srcDescriptorHandlesPtr = descriptorTableCache.second.BaseDescriptor;

			D3D12_CPU_DESCRIPTOR_HANDLE destDescriptorRangeStarts[] = {
				m_CurrentCPUDescriptorHandle
			};
			uint32_t destDescriptorRangeSizes[] = {
				numSrcDescriptors
			};

			Application::Get().GetRenderer()->GetDevice()->CopyDescriptors(1, destDescriptorRangeStarts, destDescriptorRangeSizes,
				numSrcDescriptors, srcDescriptorHandlesPtr, nullptr, m_Type);

			d3d12CommandList->SetGraphicsRootDescriptorTable(descriptorTableCache.first, m_CurrentGPUDescriptorHandle);

			m_CurrentCPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
			m_CurrentGPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
		}
	}
}

void DynamicDescriptorHeap::Reset()
{
	m_CurrentCPUDescriptorHandle = (m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart());
	m_CurrentGPUDescriptorHandle = (m_d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart());

	for (auto& descriptorTableCache : m_DescriptorTableCache)
		descriptorTableCache.second.Reset();
}
