#include "Pch.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/Buffer.h"

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize)
	: m_BufferDesc(bufferDesc), m_NumElements(numElements), m_ElementSize(elementSize)
{
	m_AlignedBufferSize = MathHelper::AlignUp(numElements * elementSize, elementSize);
	Create();
}

Buffer::Buffer(const BufferDesc& bufferDesc, std::size_t alignedSize)
	: m_BufferDesc(bufferDesc), m_AlignedBufferSize(alignedSize)
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
	auto renderer = Application::Get().GetRenderer();
	renderer->CreateBuffer(*this, m_BufferDesc.Type, m_BufferDesc.InitialState, m_AlignedBufferSize);

	if (m_BufferDesc.Type == D3D12_HEAP_TYPE_UPLOAD)
	{
		void* cpuPtr = nullptr;
		m_d3d12Resource->Map(0, nullptr, &cpuPtr);
	}
}
