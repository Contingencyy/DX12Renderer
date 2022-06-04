#include "Pch.h"
#include "Graphics/CommandList.h"
#include "Graphics/Buffer.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

CommandList::CommandList(D3D12_COMMAND_LIST_TYPE type)
	: m_d3d12CommandListType(type)
{
	auto d3d12Device = Application::Get().GetRenderer()->GetDevice()->GetD3D12Device();

	DX_CALL(d3d12Device->CreateCommandAllocator(m_d3d12CommandListType, IID_PPV_ARGS(&m_d3d12CommandAllocator)));
	DX_CALL(d3d12Device->CreateCommandList(0, m_d3d12CommandListType, m_d3d12CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));
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

void CommandList::SetPipelineState(ID3D12PipelineState* pipelineState)
{
	m_d3d12CommandList->SetPipelineState(pipelineState);
}

void CommandList::SetRootSignature(ID3D12RootSignature* rootSignature)
{
	m_d3d12CommandList->SetGraphicsRootSignature(rootSignature);
}

void CommandList::SetPrimitiveTopology(D3D12_PRIMITIVE_TOPOLOGY topology)
{
	m_d3d12CommandList->IASetPrimitiveTopology(topology);
}

void CommandList::SetDescriptorHeap(ID3D12DescriptorHeap* descriptorHeap)
{
	m_d3d12CommandList->SetDescriptorHeaps(1, &descriptorHeap);
}

void CommandList::SetShaderResourceView(ID3D12DescriptorHeap* descriptorHeap, const D3D12_CPU_DESCRIPTOR_HANDLE& srcDescriptor)
{
	m_d3d12CommandList->SetGraphicsRootDescriptorTable(1, descriptorHeap->GetGPUDescriptorHandleForHeapStart());
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
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = alignedBufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(m_d3d12CommandList.Get(), destBuffer.GetD3D12Resource().Get(),
			intermediateBuffer.GetD3D12Resource().Get(), 0, 0, 1, &subresourceData);

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destBuffer.GetD3D12Resource());
	}

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(destBuffer.GetD3D12Resource().Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	ResourceBarrier(1, &barrier);
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

		TrackObject(intermediateBuffer.GetD3D12Resource());
		TrackObject(destTexture.GetD3D12Resource());
	}

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(destTexture.GetD3D12Resource().Get(),
		D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COMMON);
	ResourceBarrier(1, &barrier);
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
}
