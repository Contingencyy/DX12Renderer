#include "Pch.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/Buffer.h"

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize, const void* data)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	m_ByteSize = MathHelper::AlignUp(numElements * elementSize, elementSize);
	Create();

	Buffer uploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_ByteSize);
	Application::Get().GetRenderer()->CopyBuffer(uploadBuffer, *this, data);
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	m_ByteSize = MathHelper::AlignUp(numElements * elementSize, elementSize);
	Create();
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t alignedSize)
	: m_BufferDesc(bufferDesc), m_ByteSize(alignedSize)
{
	Create();
}

Buffer::~Buffer()
{
	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
		m_d3d12Resource->Unmap(0, nullptr);
}

void Buffer::Create()
{
	auto device = Application::Get().GetRenderer()->GetDevice();
	device->CreateBuffer(*this, m_BufferDesc.Type, m_BufferDesc.InitialState, m_ByteSize);

	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
	{
		void* cpuPtr = nullptr;
		m_d3d12Resource->Map(0, nullptr, &cpuPtr);
	}
}
