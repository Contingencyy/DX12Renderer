#include "Pch.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Buffer.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/DescriptorHeap.h"

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
{
	auto d3d12Device = RenderBackend::GetD3D12Device();
	DX_CALL(d3d12Device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));
	DX_CALL(d3d12Device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));
	
	m_RootSignature = nullptr;
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		DescriptorHeaps[i] = nullptr;
	}
}

CommandList::~CommandList()
{
}

void CommandList::SetBackBufferAssociation(uint32_t backBufferIndex)
{
	m_BackBufferIndex = backBufferIndex;
}

void CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* clearColor)
{
	m_d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth)
{
	m_d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CommandList::SetViewports(uint32_t numViewports, D3D12_VIEWPORT* viewports)
{
	m_d3d12CommandList->RSSetViewports(numViewports, viewports);
}

void CommandList::SetScissorRects(uint32_t numRects, D3D12_RECT* rects)
{
	m_d3d12CommandList->RSSetScissorRects(numRects, rects);
}

void CommandList::SetRenderTargets(uint32_t numRTVS, D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
{
	m_d3d12CommandList->OMSetRenderTargets(numRTVS, rtvs, false, dsv);
}

void CommandList::SetRenderPassBindables(const RenderPass& renderPass)
{
	// Set the pipeline state
	m_d3d12CommandList->SetPipelineState(renderPass.GetD3D12PipelineState());

	// Set the root signature
	ID3D12RootSignature* d3d12RootSignature = renderPass.GetD3D12RootSignature();
	if (m_RootSignature != d3d12RootSignature)
	{
		m_RootSignature = d3d12RootSignature;
		m_d3d12CommandList->SetGraphicsRootSignature(d3d12RootSignature);
	}

	// Set the primitive topology
	m_d3d12CommandList->IASetPrimitiveTopology(renderPass.GetRenderPassDesc().Topology);
}

void CommandList::SetRootConstants(uint32_t rootIndex, uint32_t numValues, const void* data, uint32_t offset)
{
	m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootIndex, numValues, data, offset);
}

void CommandList::SetVertexBuffers(uint32_t slot, uint32_t numViews, const Buffer& vertexBuffer)
{
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	vbView.SizeInBytes = static_cast<uint32_t>(vertexBuffer.GetByteSize());
	vbView.StrideInBytes = static_cast<uint32_t>(vertexBuffer.GetBufferDesc().ElementSize);

	m_d3d12CommandList->IASetVertexBuffers(slot, numViews, &vbView);
}

void CommandList::SetIndexBuffer(const Buffer& indexBuffer)
{
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = indexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	ibView.SizeInBytes = static_cast<uint32_t>(indexBuffer.GetByteSize());
	ibView.Format = indexBuffer.GetBufferDesc().ElementSize == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_d3d12CommandList->IASetIndexBuffer(&ibView);
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHeap& descriptorHeap)
{
	ID3D12DescriptorHeap* d3d12DescriptorHeap = descriptorHeap.GetD3D12DescriptorHeap().Get();
	if (DescriptorHeaps[type] != d3d12DescriptorHeap)
	{
		DescriptorHeaps[type] = d3d12DescriptorHeap;

		uint32_t numDescriptorHeaps = 0;
		ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* currentHeap = DescriptorHeaps[i];
			if (currentHeap)
				descriptorHeaps[numDescriptorHeaps++] = currentHeap;
		}

		m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
	}
}

void CommandList::SetRootDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor)
{
	m_d3d12CommandList->SetGraphicsRootDescriptorTable(rootParameterIndex, baseDescriptor);
}

void CommandList::SetRootConstantBufferView(uint32_t rootParameterIndex, Buffer& buffer, D3D12_RESOURCE_STATES stateAfter)
{
	//auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(buffer.GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, 0);
	//m_d3d12CommandList->ResourceBarrier(1, &barrier);

	m_d3d12CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, buffer.GetD3D12Resource()->GetGPUVirtualAddress());
	TrackObject(buffer.GetD3D12Resource());
}

void CommandList::SetRootShaderResourceView(uint32_t rootParameterIndex, Texture& texture, D3D12_RESOURCE_STATES stateAfter)
{
	//auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, 0);
	//m_d3d12CommandList->ResourceBarrier(1, &barrier);

	m_d3d12CommandList->SetGraphicsRootShaderResourceView(rootParameterIndex, texture.GetD3D12Resource()->GetGPUVirtualAddress());
	TrackObject(texture.GetD3D12Resource());
}

void CommandList::BeginTimestampQuery(const std::string& name)
{
	uint32_t queryIndex = RenderBackend::GetNextQueryIndex(m_BackBufferIndex);
	m_d3d12CommandList->EndQuery(RenderBackend::GetD3D12TimestampQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);

	auto query = m_TimestampQueries.find(name);
	if (query == m_TimestampQueries.end())
		query = m_TimestampQueries.insert(std::pair<std::string, TimestampQuery>(name, TimestampQuery())).first;

	query->second.StartIndex = queryIndex;
	query->second.NumQueries++;
}

void CommandList::EndTimestampQuery(const std::string& name)
{
	uint32_t queryIndex = RenderBackend::GetNextQueryIndex(m_BackBufferIndex);
	m_d3d12CommandList->EndQuery(RenderBackend::GetD3D12TimestampQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP, queryIndex);

	auto query = m_TimestampQueries.find(name);
	ASSERT(query != m_TimestampQueries.end(), "A timestamp query was ended that has not been started");

	query->second.NumQueries++;
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	std::size_t alignedBufferSize = intermediateBuffer.GetByteSize();

	if (alignedBufferSize > 0 && bufferData != nullptr)
	{
		Transition(destBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = alignedBufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(m_d3d12CommandList.Get(), destBuffer.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	ASSERT(intermediateOffset + numBytes <= intermediateBuffer.GetByteSize(), "Intermediate offset is bigger than the intermediate buffer byte size");
	ASSERT(destOffset + numBytes <= destBuffer.GetByteSize(), "Destination offset is bigger than the destination buffer byte size");

	if (numBytes > 0)
	{
		Transition(destBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		m_d3d12CommandList->CopyBufferRegion(destBuffer.GetD3D12Resource().Get(), destOffset, intermediateBuffer.GetD3D12Resource().Get(), intermediateOffset, numBytes);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	TextureDesc textureDesc = destTexture.GetTextureDesc();

	if (textureData != nullptr)
	{
		Transition(destTexture, D3D12_RESOURCE_STATE_COPY_DEST);

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = textureData;
		subresourceData.RowPitch = static_cast<std::size_t>(textureDesc.Width * 4);
		subresourceData.SlicePitch = subresourceData.RowPitch * textureDesc.Height;

		UpdateSubresources(m_d3d12CommandList.Get(), destTexture.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destTexture.GetD3D12Resource());
	}
}

void CommandList::ResolveTexture(Texture& srcTexture, Texture& destTexture)
{
	Transition(srcTexture, D3D12_RESOURCE_STATE_COPY_SOURCE);
	Transition(destTexture, D3D12_RESOURCE_STATE_COPY_DEST);

	m_d3d12CommandList->CopyResource(destTexture.GetD3D12Resource().Get(), srcTexture.GetD3D12Resource().Get());

	TrackObject(destTexture.GetD3D12Resource());
	TrackObject(srcTexture.GetD3D12Resource());

	//m_d3d12CommandList->ResolveSubresource();
}

void CommandList::Transition(Resource& resource, D3D12_RESOURCE_STATES stateAfter)
{
	if (resource.GetD3D12ResourceState() != stateAfter)
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(resource.GetD3D12Resource().Get(),
			resource.GetD3D12ResourceState(), stateAfter);
		m_d3d12CommandList->ResourceBarrier(1, &barrier);

		resource.SetD3D12Resourcestate(stateAfter);
	}
}

void CommandList::TrackObject(ComPtr<ID3D12Object> object)
{
	m_TrackedObjects.push_back(object);
}

void CommandList::ReleaseTrackedObjects()
{
	m_TrackedObjects.clear();
}

void CommandList::Close()
{
	ResolveTimestampQueries();
	m_d3d12CommandList->Close();
}

void CommandList::Reset()
{
	DX_CALL(m_d3d12CommandAllocator->Reset());
	DX_CALL(m_d3d12CommandList->Reset(m_d3d12CommandAllocator.Get(), nullptr));

	ReleaseTrackedObjects();
	m_RootSignature = nullptr;

	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		DescriptorHeaps[i] = nullptr;
	}
}

void CommandList::ResolveTimestampQueries()
{
	for (auto& [name, query] : m_TimestampQueries)
	{
		std::size_t byteOffset = static_cast<std::size_t>(query.StartIndex * 8);
		m_d3d12CommandList->ResolveQueryData(RenderBackend::GetD3D12TimestampQueryHeap(), D3D12_QUERY_TYPE_TIMESTAMP,
			query.StartIndex, query.NumQueries, RenderBackend::GetQueryReadbackBuffer(m_BackBufferIndex).GetD3D12Resource().Get(), MathHelper::AlignUp(byteOffset, 8));
		RenderBackend::TrackTimestampQueryResult(name, query, m_BackBufferIndex);
	}

	m_TimestampQueries.clear();
}
