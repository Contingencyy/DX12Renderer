#include "Pch.h"
#include "Graphics/CommandList.h"
#include "Graphics/Buffer.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/DynamicDescriptorHeap.h"

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
{
	auto device = Application::Get().GetRenderer()->GetDevice();
	device->CreateCommandList(*this);
	
	m_RootSignature = nullptr;
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
	{
		m_DescriptorHeaps[i] = nullptr;
		m_DynamicDescriptorHeaps[i] = std::make_unique<DynamicDescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
	}
}

CommandList::~CommandList()
{
}

void CommandList::ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* clearColor)
{
	m_d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
}

void CommandList::ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth)
{
	m_d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, depth, 0, 0, nullptr);
}

void CommandList::SetViewports(uint32_t numViewports, const D3D12_VIEWPORT* viewports)
{
	m_d3d12CommandList->RSSetViewports(numViewports, viewports);
}

void CommandList::SetScissorRects(uint32_t numRects, const D3D12_RECT* rects)
{
	m_d3d12CommandList->RSSetScissorRects(numRects, rects);
}

void CommandList::SetRenderTargets(uint32_t numRTVs, const D3D12_CPU_DESCRIPTOR_HANDLE* rtv, const D3D12_CPU_DESCRIPTOR_HANDLE* dsv)
{
	m_d3d12CommandList->OMSetRenderTargets(numRTVs, rtv, false, dsv);
}

void CommandList::SetPipelineState(const PipelineState& pipelineState)
{
	m_d3d12CommandList->SetPipelineState(pipelineState.GetD3D12PipelineState().Get());

	ID3D12RootSignature* d3d12RootSignature = pipelineState.GetRootSignature()->GetD3D12RootSignature().Get();
	if (m_RootSignature != d3d12RootSignature)
	{
		m_RootSignature = d3d12RootSignature;
		m_d3d12CommandList->SetGraphicsRootSignature(d3d12RootSignature);

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			m_DynamicDescriptorHeaps[i]->ParseRootSignature(*pipelineState.GetRootSignature());
		}
	}

	m_d3d12CommandList->IASetPrimitiveTopology(pipelineState.GetPrimitiveTopology());
}

void CommandList::SetRoot32BitConstants(uint32_t rootIndex, uint32_t numValues, const void* data, uint32_t offset)
{
	m_d3d12CommandList->SetGraphicsRoot32BitConstants(rootIndex, numValues, data, offset);
}

void CommandList::SetVertexBuffers(uint32_t slot, uint32_t numViews, const Buffer& vertexBuffer)
{
	D3D12_VERTEX_BUFFER_VIEW vbView = {};
	vbView.BufferLocation = vertexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	vbView.SizeInBytes = vertexBuffer.GetByteSize();
	vbView.StrideInBytes = vertexBuffer.GetElementSize();

	m_d3d12CommandList->IASetVertexBuffers(slot, numViews, &vbView);
}

void CommandList::SetIndexBuffer(const Buffer& indexBuffer)
{
	D3D12_INDEX_BUFFER_VIEW ibView = {};
	ibView.BufferLocation = indexBuffer.GetD3D12Resource()->GetGPUVirtualAddress();
	ibView.SizeInBytes = indexBuffer.GetByteSize();
	ibView.Format = indexBuffer.GetElementSize() == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT;

	m_d3d12CommandList->IASetIndexBuffer(&ibView);
}

void CommandList::SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, ID3D12DescriptorHeap* heap)
{
	if (m_DescriptorHeaps[type] != heap)
	{
		m_DescriptorHeaps[type] = heap;

		uint32_t numDescriptorHeaps = 0;
		ID3D12DescriptorHeap* descriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES] = {};

		for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		{
			ID3D12DescriptorHeap* descriptorHeap = m_DescriptorHeaps[i];
			if (descriptorHeap)
				descriptorHeaps[numDescriptorHeaps++] = descriptorHeap;
		}

		m_d3d12CommandList->SetDescriptorHeaps(numDescriptorHeaps, descriptorHeaps);
	}
}

void CommandList::SetShaderResourceView(uint32_t rootParameterIndex, uint32_t descriptorOffset, Texture& texture,
	D3D12_RESOURCE_STATES stateAfter, const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc)
{
	auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(texture.GetD3D12Resource().Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, 0);
	m_d3d12CommandList->ResourceBarrier(1, &barrier);

	m_DynamicDescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, texture.GetDescriptorHandle());
}

void CommandList::Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex, uint32_t startInstance)
{
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		m_DynamicDescriptorHeaps[i]->CommitStagedDescriptors(*this);

	m_d3d12CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}

void CommandList::DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex, int32_t baseVertex, uint32_t startInstance)
{
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		m_DynamicDescriptorHeaps[i]->CommitStagedDescriptors(*this);

	m_d3d12CommandList->DrawIndexedInstanced(indexCount, instanceCount, startIndex, baseVertex, startInstance);
}

void CommandList::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
	std::size_t alignedBufferSize = intermediateBuffer.GetByteSize();

	if (alignedBufferSize > 0 && bufferData != nullptr)
	{
		CD3DX12_RESOURCE_BARRIER copyDestBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST);
		ResourceBarrier(1, &copyDestBarrier);

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = alignedBufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(m_d3d12CommandList.Get(), destBuffer.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		ResourceBarrier(1, &commonBarrier);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
	TextureDesc textureDesc = destTexture.GetTextureDesc();

	if (textureData != nullptr)
	{
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = textureData;
		subresourceData.RowPitch = static_cast<std::size_t>(textureDesc.Width * 4);
		subresourceData.SlicePitch = subresourceData.RowPitch * textureDesc.Height;

		UpdateSubresources(m_d3d12CommandList.Get(), destTexture.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(destTexture.GetD3D12Resource().Get(),
			D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
		ResourceBarrier(1, &barrier);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destTexture.GetD3D12Resource());
	}
}

void CommandList::ResolveTexture(Texture& srcTexture, Texture& destTexture)
{
	m_d3d12CommandList->CopyResource(destTexture.GetD3D12Resource().Get(), srcTexture.GetD3D12Resource().Get());

	TrackObject(destTexture.GetD3D12Resource());
	TrackObject(srcTexture.GetD3D12Resource());

	//m_d3d12CommandList->ResolveSubresource();
}

void CommandList::ResourceBarrier(uint32_t numBarriers, const D3D12_RESOURCE_BARRIER* barriers)
{
	m_d3d12CommandList->ResourceBarrier(numBarriers, barriers);
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
		m_DescriptorHeaps[i] = nullptr;
		m_DynamicDescriptorHeaps[i]->Reset();
	}
}
