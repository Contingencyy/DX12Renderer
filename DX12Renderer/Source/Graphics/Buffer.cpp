#include "Pch.h"
#include "Graphics/Buffer.h"
#include "Graphics/Backend/RenderBackend.h"

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc)
	: Resource(name), m_BufferDesc(bufferDesc)
{
	if (IsValid())
	{
		CreateD3D12Resource();
		AllocateDescriptors();
		CreateViews();
		SetName(name);
	}
}

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc, const void* data)
	: Resource(name), m_BufferDesc(bufferDesc)
{
	if (IsValid())
	{
		CreateD3D12Resource();
		AllocateDescriptors();
		CreateViews();
		SetName(name);

		SetBufferData(data);
	}
}

Buffer::~Buffer()
{
	if (IsCPUAccessible())
		m_d3d12Resource->Unmap(0, nullptr);
}

bool Buffer::IsValid() const
{
	return m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_NONE;
}

bool Buffer::IsCPUAccessible() const
{
	switch (m_BufferDesc.Usage)
	{
	case BufferUsage::BUFFER_USAGE_CONSTANT:
	case BufferUsage::BUFFER_USAGE_UPLOAD:
	case BufferUsage::BUFFER_USAGE_READBACK:
		return true;
	default:
		return false;
	}
}

void Buffer::Invalidate()
{
	if (IsValid())
	{
		CreateD3D12Resource();

		ResetDescriptorAllocations();
		AllocateDescriptors();

		CreateViews();
		SetName(m_Name);
	}
}

void Buffer::SetBufferData(const void* data, std::size_t byteSize)
{
	std::size_t dataByteSize = byteSize == 0 ? m_ByteSize : byteSize;
	if (IsCPUAccessible())
		memcpy(m_CPUPtr, data, dataByteSize);
	else
		RenderBackend::UploadBufferData(*this, data);
}

void Buffer::SetBufferDataAtOffset(const void* data, std::size_t byteSize, std::size_t byteOffset)
{
	if (IsCPUAccessible())
		memcpy(static_cast<unsigned char*>(m_CPUPtr) + byteOffset, data, byteSize);
	else
		RenderBackend::UploadBufferDataRegion(*this, byteOffset, byteSize);
}

void Buffer::CreateD3D12Resource()
{
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	uint32_t alignment = static_cast<uint32_t>(m_BufferDesc.ElementSize);

	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT)
	{
		alignment = 256;
	}

	m_ByteSize = MathHelper::AlignUp(m_BufferDesc.NumElements * m_BufferDesc.ElementSize, alignment);

	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		m_d3d12ResourceState = D3D12_RESOURCE_STATE_GENERIC_READ;
	}
	else if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_READBACK)
	{
		heapType = D3D12_HEAP_TYPE_READBACK;
		m_d3d12ResourceState = D3D12_RESOURCE_STATE_COPY_DEST;
	}

	CD3DX12_RESOURCE_DESC d3d12ResourceDesc = CD3DX12_RESOURCE_DESC::Buffer(m_ByteSize);
	RenderBackend::CreateBuffer(m_d3d12Resource, heapType, d3d12ResourceDesc, m_d3d12ResourceState);

	if (IsCPUAccessible())
	{
		m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
	}
}

void Buffer::AllocateDescriptors()
{
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_READ)
	{
		m_DescriptorAllocations[DescriptorType::SRV] = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_WRITE)
	{
		m_DescriptorAllocations[DescriptorType::UAV] = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT)
	{
		m_DescriptorAllocations[DescriptorType::CBV] = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	}
}

void Buffer::CreateViews()
{
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_READ)
	{
		// Create shader resource view (read-only)
		auto& srvAllocation = m_DescriptorAllocations[DescriptorType::SRV];

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Format = DXGI_FORMAT_UNKNOWN;
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Buffer.FirstElement = 0;
		srvDesc.Buffer.NumElements = static_cast<uint32_t>(m_BufferDesc.NumElements);
		srvDesc.Buffer.StructureByteStride = static_cast<uint32_t>(m_BufferDesc.ElementSize);
		srvDesc.Buffer.Flags = D3D12_BUFFER_SRV_FLAG_NONE;

		RenderBackend::GetD3D12Device()->CreateShaderResourceView(m_d3d12Resource.Get(), &srvDesc, srvAllocation.GetCPUDescriptorHandle());
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_WRITE)
	{
		// Create unordered access view (read/write)
		auto& uavAllocation = m_DescriptorAllocations[DescriptorType::UAV];

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Format = DXGI_FORMAT_R32_FLOAT;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = static_cast<uint32_t>(m_BufferDesc.NumElements);
		uavDesc.Buffer.StructureByteStride = static_cast<uint32_t>(m_BufferDesc.ElementSize);
		uavDesc.Buffer.CounterOffsetInBytes = 0;
		uavDesc.Buffer.Flags = D3D12_BUFFER_UAV_FLAG_NONE;

		RenderBackend::GetD3D12Device()->CreateUnorderedAccessView(m_d3d12Resource.Get(), nullptr, &uavDesc, uavAllocation.GetCPUDescriptorHandle());
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT)
	{
		// Create constant buffer view
		auto& cbvAllocation = m_DescriptorAllocations[DescriptorType::CBV];

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_ByteSize;

		RenderBackend::GetD3D12Device()->CreateConstantBufferView(&cbvDesc, cbvAllocation.GetCPUDescriptorHandle());
	}
}
