#include "Pch.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Buffer.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/UploadBuffer.h"

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

void CommandList::CopyBuffer(const UploadBufferAllocation& uploadBuffer, Buffer& destBuffer, const void* bufferData)
{
	std::size_t alignedBufferSize = destBuffer.GetByteSize();

	if (alignedBufferSize > 0 && bufferData != nullptr)
	{
		Transition(destBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = bufferData;
		subresourceData.RowPitch = alignedBufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;

		UpdateSubresources(m_d3d12CommandList.Get(), destBuffer.GetD3D12Resource().Get(),
			uploadBuffer.D3D12Resource, uploadBuffer.OffsetInBuffer, 0, 1, &subresourceData);

		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyBufferRegion(const UploadBufferAllocation& uploadBuffer, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	ASSERT(destOffset + numBytes <= destBuffer.GetByteSize(), "Destination offset is bigger than the destination buffer byte size");
	std::size_t alignedBufferSize = destBuffer.GetByteSize();

	if (numBytes > 0)
	{
		Transition(destBuffer, D3D12_RESOURCE_STATE_COPY_DEST);

		m_d3d12CommandList->CopyBufferRegion(destBuffer.GetD3D12Resource().Get(), destOffset,
			uploadBuffer.D3D12Resource, uploadBuffer.OffsetInBuffer, numBytes);

		TrackObject(destBuffer.GetD3D12Resource());
	}
}

void CommandList::CopyTexture(const UploadBufferAllocation& uploadBuffer, Texture& destTexture, const void* textureData)
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
			uploadBuffer.D3D12Resource, uploadBuffer.OffsetInBuffer, 0, 1, &subresourceData);

		TrackObject(destTexture.GetD3D12Resource());
	}
}

void CommandList::GenerateMips(Texture& texture)
{
	auto srcResource = texture.GetD3D12Resource();
	if (!srcResource)
		return;

	auto srcResourceDesc = srcResource->GetDesc();
	if (srcResourceDesc.MipLevels == 1)
		return;

	if (srcResourceDesc.Dimension != D3D12_RESOURCE_DIMENSION_TEXTURE2D ||
		srcResourceDesc.DepthOrArraySize != 1 ||
		srcResourceDesc.SampleDesc.Count > 1)
	{
		ASSERT(false, "Generating mips for texture dimensions that are not TEXTURE2D, texture arrays, or multisampled textures are not supported");
		return;
	}
	
	ID3D12Device2* d3d12Device = RenderBackend::GetD3D12Device();
	ComPtr<ID3D12Resource> uavResource = srcResource;
	ComPtr<ID3D12Resource> aliasResource;

	// Create a copy of the texture which will be used to alias the source texture, if unordered access is not allowed on the source texture
	if ((srcResourceDesc.Flags & D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS) == 0)
	{
		D3D12_RESOURCE_DESC aliasDesc = srcResourceDesc;
		aliasDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		aliasDesc.Flags &= ~(D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET | D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

		D3D12_RESOURCE_DESC uavDesc = aliasDesc;
		D3D12_RESOURCE_DESC aliasUAVDescs[] = { aliasDesc, uavDesc };

		D3D12_RESOURCE_ALLOCATION_INFO allocationInfo = d3d12Device->GetResourceAllocationInfo(0, _countof(aliasUAVDescs), aliasUAVDescs);

		D3D12_HEAP_DESC heapDesc = {};
		heapDesc.SizeInBytes = allocationInfo.SizeInBytes;
		heapDesc.Alignment = allocationInfo.Alignment;
		heapDesc.Flags = D3D12_HEAP_FLAG_ALLOW_ONLY_NON_RT_DS_TEXTURES;
		heapDesc.Properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
		heapDesc.Properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
		heapDesc.Properties.Type = D3D12_HEAP_TYPE_DEFAULT;

		ComPtr<ID3D12Heap> heap;
		DX_CALL(d3d12Device->CreateHeap(&heapDesc, IID_PPV_ARGS(&heap)));
		TrackObject(heap);

		DX_CALL(d3d12Device->CreatePlacedResource(
			heap.Get(),
			0,
			&aliasDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&aliasResource)
		));
		TrackObject(aliasResource);

		DX_CALL(d3d12Device->CreatePlacedResource(
			heap.Get(),
			0,
			&uavDesc,
			D3D12_RESOURCE_STATE_COMMON,
			nullptr,
			IID_PPV_ARGS(&uavResource)
		));
		TrackObject(uavResource);

		CD3DX12_RESOURCE_BARRIER aliasBarrier = CD3DX12_RESOURCE_BARRIER::Aliasing(nullptr, aliasResource.Get());
		m_d3d12CommandList->ResourceBarrier(1, &aliasBarrier);

		m_d3d12CommandList->CopyResource(aliasResource.Get(), srcResource.Get());

		CD3DX12_RESOURCE_BARRIER uavAliasBarrier = CD3DX12_RESOURCE_BARRIER::Aliasing(aliasResource.Get(), uavResource.Get());
		m_d3d12CommandList->ResourceBarrier(1, &uavAliasBarrier);
	}

	struct MipGenCB
	{
		uint32_t SrcMipIndex;  // The source texture index in the descriptor heap
		uint32_t DestMipIndex; // The destination texture index in the descriptor heap
		uint32_t SrcMipLevel;  // The level of the source mip
		uint32_t NumMips;      // Number of mips to generate in current dispatch
		uint32_t SrcDimension; // Bitmask to specify if source mip is odd in width and/or height
		uint32_t IsSRGB;	   // Is the texture in SRGB color space? Apply gamma correction
		glm::vec2 TexelSize;   // 1.0 / OutMip0.Dimensions
	} mipGenCB;

	DescriptorAllocation srvDescriptors = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, 1);
	DescriptorAllocation uavDescriptors = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, srcResourceDesc.MipLevels);

	m_d3d12CommandList->SetPipelineState(RenderBackend::GetMipGenPSO());
	ID3D12RootSignature* d3d12RootSignature = RenderBackend::GetMipGenRootSig();
	if (m_RootSignature != d3d12RootSignature)
	{
		m_RootSignature = d3d12RootSignature;
		m_d3d12CommandList->SetComputeRootSignature(d3d12RootSignature);
	}

	mipGenCB.IsSRGB = 0;

	for (uint32_t srcMip = 0; srcMip < (srcResourceDesc.MipLevels - 1);)
	{
		uint64_t srcWidth = srcResourceDesc.Width >> srcMip;
		uint32_t srcHeight = srcResourceDesc.Height >> srcMip;
		uint32_t destWidth = static_cast<uint32_t>(srcWidth >> 1);
		uint32_t destHeight = srcHeight >> 1;

		mipGenCB.SrcDimension = (srcHeight & 1) << 1 | (srcWidth & 1);
		DWORD mipCount = 0;

		_BitScanForward(&mipCount, (destWidth == 1 ? destHeight : destWidth) |
			(destHeight == 1 ? destWidth : destHeight));

		mipCount = std::min<DWORD>(4, mipCount + 1);
		mipCount = (srcMip + mipCount) >= srcResourceDesc.MipLevels ?
			srcResourceDesc.MipLevels - srcMip - 1 : mipCount;

		destWidth = std::max(1u, destWidth);
		destHeight = std::max(1u, destHeight);

		mipGenCB.SrcMipLevel = srcMip;
		mipGenCB.NumMips = mipCount;
		mipGenCB.TexelSize.x = 1.0f / (float)destWidth;
		mipGenCB.TexelSize.y = 1.0f / (float)destHeight;

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Format = srcResourceDesc.Format;
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Texture2D.MostDetailedMip = 0;
		srvDesc.Texture2D.MipLevels = srcResourceDesc.MipLevels;

		d3d12Device->CreateShaderResourceView(uavResource.Get(), &srvDesc, srvDescriptors.GetCPUDescriptorHandle());

		for (uint32_t mip = 0; mip < mipCount; ++mip)
		{
			D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
			uavDesc.Format = srcResourceDesc.Format;
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
			uavDesc.Texture2D.MipSlice = srcMip + mip + 1;

			d3d12Device->CreateUnorderedAccessView(uavResource.Get(), nullptr, &uavDesc,
				{ uavDescriptors.GetCPUDescriptorHandle().ptr + ((srcMip + mip) * d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)) });
		}

		mipGenCB.SrcMipIndex = srvDescriptors.GetOffsetInDescriptorHeap();
		mipGenCB.DestMipIndex = uavDescriptors.GetOffsetInDescriptorHeap() + srcMip;

		m_d3d12CommandList->SetComputeRoot32BitConstants(0, 8, &mipGenCB, 0);

		SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV));
		m_d3d12CommandList->SetComputeRootDescriptorTable(1, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());
		m_d3d12CommandList->SetComputeRootDescriptorTable(2, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());

		m_d3d12CommandList->Dispatch(MathHelper::DivideUp(destWidth, 8u), MathHelper::DivideUp(destHeight, 8u), 1);
		
		CD3DX12_RESOURCE_BARRIER uavBarrier = CD3DX12_RESOURCE_BARRIER::UAV(uavResource.Get());
		m_d3d12CommandList->ResourceBarrier(1, &uavBarrier);

		srcMip += mipCount;
	}

	if (aliasResource)
	{
		CD3DX12_RESOURCE_BARRIER aliasCopyBarriers[3] = {
			CD3DX12_RESOURCE_BARRIER::Aliasing(uavResource.Get(), aliasResource.Get()),
			CD3DX12_RESOURCE_BARRIER::Transition(aliasResource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE),
			CD3DX12_RESOURCE_BARRIER::Transition(srcResource.Get(),
				D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_COPY_DEST),
		};
		m_d3d12CommandList->ResourceBarrier(3, &aliasCopyBarriers[0]);

		m_d3d12CommandList->CopyResource(srcResource.Get(), aliasResource.Get());

		if (texture.GetD3D12ResourceState() != D3D12_RESOURCE_STATE_COPY_DEST)
		{
			CD3DX12_RESOURCE_BARRIER previousStateBarrier = CD3DX12_RESOURCE_BARRIER::Transition(srcResource.Get(),
				D3D12_RESOURCE_STATE_COPY_DEST, texture.GetD3D12ResourceState());
			m_d3d12CommandList->ResourceBarrier(1, &previousStateBarrier);
		}
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
