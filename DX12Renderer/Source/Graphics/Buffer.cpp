#include "Pch.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/Buffer.h"

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize, const void* data)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	m_ByteSize = MathHelper::AlignUp(m_NumElements * m_ElementSize, m_ElementSize);
	Create();

	SetBufferData(data);
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	m_ByteSize = MathHelper::AlignUp(m_NumElements * m_ElementSize, m_ElementSize);
	Create();
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t alignedSize)
	: m_BufferDesc(bufferDesc), m_NumElements(1), m_ElementSize(alignedSize), m_ByteSize(alignedSize)
{
	Create();
}

Buffer::~Buffer()
{
	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
		m_d3d12Resource->Unmap(0, nullptr);
}

void Buffer::SetBufferData(const void* data, uint32_t numElements)
{
	if (m_BufferDesc.Type != D3D12_HEAP_TYPE_UPLOAD)
	{
		uint32_t byteSize = numElements == 0 ? m_ByteSize : numElements * m_ElementSize;

		Buffer uploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), byteSize);
		Application::Get().GetRenderer()->CopyBuffer(uploadBuffer, *this, data);
	}
	else
	{
		LOG_ERR("Should not call this SetBufferData function for upload heaps");
	}
}

void Buffer::SetBufferDataStaging(const void* data, std::size_t alignedSize)
{
	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
	{
		std::size_t byteSize = alignedSize == 0 ? m_ByteSize : alignedSize;
		memcpy(m_CPUPtr, data, alignedSize);
	}
	else
	{
		LOG_ERR("Should not call this SetBufferData function for non-upload heaps");
	}
}

void Buffer::Create()
{
	auto device = Application::Get().GetRenderer()->GetDevice();
	device->CreateBuffer(*this, m_BufferDesc.Type, m_BufferDesc.InitialState, m_ByteSize);

	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
	{
		m_d3d12Resource->Map(0, nullptr, &m_CPUPtr);
	}
}
