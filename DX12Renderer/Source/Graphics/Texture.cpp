#include "Pch.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/Texture.h"

Texture::Texture(const TextureDesc& textureDesc)
	: m_TextureDesc(textureDesc)
{
	m_DescriptorHandle.ptr = 0;
	Create();
}

Texture::~Texture()
{
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDescriptorHandle()
{
	auto d3d12Device = Application::Get().GetRenderer()->GetD3D12Device();

	if (m_DescriptorHandle.ptr == 0)
	{
		switch (m_TextureDesc.Flags)
		{
		case D3D12_RESOURCE_FLAG_NONE:
		case D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:
		{
			D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
			srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
			srvDesc.Format = m_d3d12Resource->GetDesc().Format;
			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
			srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

			m_DescriptorHandle = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			d3d12Device->CreateShaderResourceView(m_d3d12Resource.Get(), &srvDesc, m_DescriptorHandle);
			break;
		}
		case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
		{
			m_DescriptorHandle = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			d3d12Device->CreateRenderTargetView(m_d3d12Resource.Get(), nullptr, m_DescriptorHandle);
			break;
		}
		case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
		{
			m_DescriptorHandle = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			d3d12Device->CreateDepthStencilView(m_d3d12Resource.Get(), nullptr, m_DescriptorHandle);
			break;
		}
		}
	}

	return m_DescriptorHandle;
}

void Texture::Create()
{
	D3D12_RESOURCE_DESC d3d12ResourceDesc = {};
	d3d12ResourceDesc.MipLevels = 1;
	d3d12ResourceDesc.Width = m_TextureDesc.Width;
	d3d12ResourceDesc.Height = m_TextureDesc.Height;
	d3d12ResourceDesc.DepthOrArraySize = 1;
	d3d12ResourceDesc.SampleDesc.Count = 1;
	d3d12ResourceDesc.SampleDesc.Quality = 0;
	d3d12ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	d3d12ResourceDesc.Format = m_TextureDesc.Format;
	d3d12ResourceDesc.Flags = m_TextureDesc.Flags;

	m_AlignedBufferSize = MathHelper::AlignUp(static_cast<std::size_t>(m_TextureDesc.Width * m_TextureDesc.Height * 4), 4);

	if (m_TextureDesc.Flags == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = m_TextureDesc.Format;
		clearValue.DepthStencil = { 1.0f, 0 };

		Application::Get().GetRenderer()->CreateTexture(*this, d3d12ResourceDesc, m_TextureDesc.InitialState, m_AlignedBufferSize, &clearValue);
	}
	else
	{
		Application::Get().GetRenderer()->CreateTexture(*this, d3d12ResourceDesc, m_TextureDesc.InitialState, m_AlignedBufferSize);
	}
}
