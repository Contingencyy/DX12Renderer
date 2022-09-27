#include "Pch.h"
#include "Graphics/Buffer.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/RenderBackend.h"

D3D12_RESOURCE_STATES BufferUsageToD3DResourceState(BufferUsage usage)
{
	switch (usage)
	{
	case BufferUsage::BUFFER_USAGE_VERTEX:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
	case BufferUsage::BUFFER_USAGE_INDEX:
		return D3D12_RESOURCE_STATE_INDEX_BUFFER;
	case BufferUsage::BUFFER_USAGE_READ:
		return D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE | D3D12_RESOURCE_STATE_NON_PIXEL_SHADER_RESOURCE;
	case BufferUsage::BUFFER_USAGE_WRITE:
		return D3D12_RESOURCE_STATE_UNORDERED_ACCESS;
	case BufferUsage::BUFFER_USAGE_CONSTANT:
		return D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER | D3D12_RESOURCE_STATE_GENERIC_READ;
	case BufferUsage::BUFFER_USAGE_UPLOAD:
		return D3D12_RESOURCE_STATE_GENERIC_READ;
	}

	LOG_ERR("Buffer usage is not supported");
	return D3D12_RESOURCE_STATE_COMMON;
}

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc, const void* data)
	: m_BufferDesc(bufferDesc)
{
	if (IsValid())
	{
		Create();
		CreateViews();
		SetName(name);
		SetBufferData(data);
	}
}

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc)
	: m_BufferDesc(bufferDesc)
{
	if (IsValid())
	{
		Create();
		CreateViews();
		SetName(name);
	}
}

Buffer::~Buffer()
{
	if (m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_UPLOAD)
		m_d3d12Resource->Unmap(0, nullptr);
}

void Buffer::SetBufferData(const void* data, std::size_t byteSize)
{
	std::size_t dataByteSize = byteSize == 0 ? m_ByteSize : byteSize;
	if (m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_CONSTANT && m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		Buffer uploadBuffer(m_Name + " - Upload buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, dataByteSize));
		RenderBackend::CopyBuffer(uploadBuffer, *this, data);
	}
	else
	{
		memcpy(m_CPUPtr, data, dataByteSize);
	}
}

void Buffer::SetBufferDataAtOffset(const void* data, std::size_t byteSize, std::size_t byteOffset)
{
	if (m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_CONSTANT && m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		Buffer uploadBuffer(m_Name + " - Upload buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, byteSize));
		RenderBackend::CopyBufferRegion(uploadBuffer, 0, *this, byteOffset, byteSize);
	}
	else
	{
		memcpy(static_cast<unsigned char*>(m_CPUPtr) + byteOffset, data, byteSize);
	}
}

bool Buffer::IsValid() const
{
	return m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_NONE;
}

D3D12_CPU_DESCRIPTOR_HANDLE Buffer::GetDescriptorHandle(DescriptorType type) const
{
	return m_DescriptorAllocations[type].GetCPUDescriptorHandle();
}

uint32_t Buffer::GetDescriptorIndex(DescriptorType type) const
{
	return m_DescriptorAllocations[type].GetOffsetInDescriptorHeap();
}

void Buffer::SetName(const std::string& name)
{
	m_Name = name;
	m_d3d12Resource->SetName(StringHelper::StringToWString(name).c_str());
}

void Buffer::Create()
{
	auto device = RenderBackend::GetDevice();
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	//D3D12_RESOURCE_STATES initialState = BufferUsageToD3DResourceState(m_BufferDesc.Usage);
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (m_BufferDesc.Usage)
	{
	case BufferUsage::BUFFER_USAGE_VERTEX:
	case BufferUsage::BUFFER_USAGE_INDEX:
	case BufferUsage::BUFFER_USAGE_READ:
	case BufferUsage::BUFFER_USAGE_WRITE:
		m_ByteSize = MathHelper::AlignUp(m_BufferDesc.NumElements * m_BufferDesc.ElementSize, m_BufferDesc.ElementSize);
		break;
	case BufferUsage::BUFFER_USAGE_CONSTANT:
		m_ByteSize = MathHelper::AlignUp(m_BufferDesc.NumElements * m_BufferDesc.ElementSize, 256);
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;
	case BufferUsage::BUFFER_USAGE_UPLOAD:
		m_ByteSize = MathHelper::AlignUp(m_BufferDesc.NumElements * m_BufferDesc.ElementSize, m_BufferDesc.ElementSize);
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		initialState = D3D12_RESOURCE_STATE_GENERIC_READ;
		break;
	}

	device->CreateBuffer(*this, heapType, initialState, m_ByteSize);

	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
	}
}

void Buffer::CreateViews()
{
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_READ)
	{
		auto& srv = m_DescriptorAllocations[DescriptorType::SRV];

		// Create shader resource view (read-only)
		if (srv.IsNull())
			srv = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		//srvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
		srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

		RenderBackend::GetDevice()->CreateShaderResourceView(*this, srvDesc, srv.GetCPUDescriptorHandle());
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_WRITE)
	{
		auto& uav = m_DescriptorAllocations[DescriptorType::UAV];

		// Create unordered access view (read/write)
		if (uav.IsNull())
			uav = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		
		//uavDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = 1;
		uavDesc.Buffer.CounterOffsetInBytes = 0;

		RenderBackend::GetDevice()->CreateUnorderedAccessView(*this, uavDesc, uav.GetCPUDescriptorHandle());
	}
	if (m_BufferDesc.Usage & BufferUsage::BUFFER_USAGE_CONSTANT)
	{
		auto& cbv = m_DescriptorAllocations[DescriptorType::CBV];

		if (cbv.IsNull())
			cbv = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_ByteSize;

		RenderBackend::GetDevice()->CreateConstantBufferView(*this, cbvDesc, cbv.GetCPUDescriptorHandle());
	}
}
