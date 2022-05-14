#include "Pch.h"
#include "Graphics/Buffer.h"

Buffer::Buffer()
{
}

Buffer::~Buffer()
{
}

ComPtr<ID3D12Resource> Buffer::GetD3D12Resource() const
{
	return m_d3d12Resource;
}

void Buffer::SetD3D12Resource(ComPtr<ID3D12Resource> resource)
{
	m_d3d12Resource = resource;
}
