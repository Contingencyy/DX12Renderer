#include "Pch.h"
#include "Graphics/Device.h"
#include "Graphics/Buffer.h"
#include "Graphics/RenderBackend.h"

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc, const void* data)
	: m_BufferDesc(bufferDesc)
{
	Create();
	SetName(name);
	SetBufferData(data);
}

Buffer::Buffer(const std::string& name, const BufferDesc& bufferDesc)
	: m_BufferDesc(bufferDesc)
{
	Create();
	SetName(name);
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
		RenderBackend::Get().CopyBuffer(uploadBuffer, *this, data);
	}
	else
	{
		memcpy(m_CPUPtr, data, dataByteSize);
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Buffer::GetDescriptorHandle()
{
	if (m_DescriptorAllocation.IsNull())
	{
		m_DescriptorAllocation = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_ByteSize;

		RenderBackend::Get().GetDevice()->CreateConstantBufferView(*this, cbvDesc, m_DescriptorAllocation.GetCPUDescriptorHandle());
	}

	return m_DescriptorAllocation.GetCPUDescriptorHandle();
}

void Buffer::SetName(const std::string& name)
{
	m_Name = name;
	m_d3d12Resource->SetName(StringHelper::StringToWString(name).c_str());
}

void Buffer::Create()
{
	auto device = RenderBackend::Get().GetDevice();
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	switch (m_BufferDesc.Usage)
	{
	case BufferUsage::BUFFER_USAGE_VERTEX:
	case BufferUsage::BUFFER_USAGE_INDEX:
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
	if (m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
	}
}
