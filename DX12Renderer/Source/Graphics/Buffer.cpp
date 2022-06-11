#include "Pch.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/Buffer.h"

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize, const void* data)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	Create();

	SetBufferData(data);
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	Create();
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t alignedSize)
	: m_BufferDesc(bufferDesc), m_NumElements(1), m_ElementSize(alignedSize), m_ByteSize(alignedSize)
{
	Create();
}

Buffer::~Buffer()
{
	if (m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_UPLOAD)
		m_d3d12Resource->Unmap(0, nullptr);
}

void Buffer::SetBufferData(const void* data, uint32_t numElements)
{
	if (m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_CONSTANT && m_BufferDesc.Usage != BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		uint32_t byteSize = numElements == 0 ? m_ByteSize : numElements * m_ElementSize;

		Buffer uploadBuffer(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), byteSize);
		Application::Get().GetRenderer()->CopyBuffer(uploadBuffer, *this, data);
	}
	else
	{
		LOG_ERR("Should not call this SetBufferData function for upload heaps");
	}
}

void Buffer::SetBufferDataStaging(const void* data, std::size_t alignedSize)
{
	if (m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		std::size_t byteSize = alignedSize == 0 ? m_ByteSize : alignedSize;
		memcpy(m_CPUPtr, data, alignedSize);
	}
	else
	{
		LOG_ERR("Should not call this SetBufferData function for non-upload heaps");
	}
}

D3D12_CPU_DESCRIPTOR_HANDLE Buffer::GetDescriptorHandle()
{
	if (m_DescriptorAllocation.IsNull())
	{
		m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
		cbvDesc.BufferLocation = m_d3d12Resource->GetGPUVirtualAddress();
		cbvDesc.SizeInBytes = m_ByteSize;

		Application::Get().GetRenderer()->GetDevice()->CreateConstantBufferView(*this, cbvDesc, m_DescriptorAllocation.GetDescriptorHandle());
	}

	return m_DescriptorAllocation.GetDescriptorHandle();
}

void Buffer::Create()
{
	auto device = Application::Get().GetRenderer()->GetDevice();
	D3D12_HEAP_TYPE heapType = D3D12_HEAP_TYPE_DEFAULT;

	switch (m_BufferDesc.Usage)
	{
	case BufferUsage::BUFFER_USAGE_VERTEX:
	case BufferUsage::BUFFER_USAGE_INDEX:
		m_ByteSize = MathHelper::AlignUp(m_NumElements * m_ElementSize, m_ElementSize);
		heapType = D3D12_HEAP_TYPE_DEFAULT;
		break;
	case BufferUsage::BUFFER_USAGE_CONSTANT:
		m_ByteSize = MathHelper::AlignUp(m_NumElements * m_ElementSize, 256);
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		break;
	case BufferUsage::BUFFER_USAGE_UPLOAD:
		m_ByteSize = MathHelper::AlignUp(m_NumElements * m_ElementSize, m_ElementSize);
		heapType = D3D12_HEAP_TYPE_UPLOAD;
		break;
	}

	device->CreateBuffer(*this, heapType, m_BufferDesc.InitialState, m_ByteSize);
	if (m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_CONSTANT || m_BufferDesc.Usage == BufferUsage::BUFFER_USAGE_UPLOAD)
	{
		m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
	}
}
