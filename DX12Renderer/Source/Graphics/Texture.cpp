#include "Pch.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/Texture.h"

Texture::Texture(const TextureDesc& textureDesc, const void* data)
	: m_TextureDesc(textureDesc)
{
	Create();

	Buffer uploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_ByteSize);
	Application::Get().GetRenderer()->CopyTexture(uploadBuffer, *this, data);
}

Texture::Texture(const TextureDesc& textureDesc)
	: m_TextureDesc(textureDesc)
{
	Create();
}

Texture::~Texture()
{
}

void Texture::Resize(uint32_t width, uint32_t height)
{
	m_TextureDesc.Width = width;
	m_TextureDesc.Height = height;

	Create();
	CreateView();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDescriptorHandle()
{
	if (m_DescriptorAllocation.IsNull())
	{
		switch (m_TextureDesc.Flags)
		{
		case D3D12_RESOURCE_FLAG_NONE:
		case D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS:
		{
			m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			break;
		}
		case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
		{
			m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			break;
		}
		case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
		{
			m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
			break;
		}
		}

		CreateView();
	}

	return m_DescriptorAllocation.GetDescriptorHandle();
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

	if (m_TextureDesc.Flags == D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL)
	{
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = m_TextureDesc.Format;
		clearValue.DepthStencil = { 1.0f, 0 };

		Application::Get().GetRenderer()->GetDevice()->CreateTexture(*this, d3d12ResourceDesc, m_TextureDesc.InitialState, &clearValue);
	}
	else
	{
		Application::Get().GetRenderer()->GetDevice()->CreateTexture(*this, d3d12ResourceDesc, m_TextureDesc.InitialState);
	}

	m_ByteSize = GetRequiredIntermediateSize(m_d3d12Resource.Get(), 0, 1);
}

void Texture::CreateView()
{
	auto device = Application::Get().GetRenderer()->GetDevice();

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

		device->CreateShaderResourceView(*this, srvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	case D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET:
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = m_TextureDesc.Format;
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D = D3D12_TEX2D_RTV();

		device->CreateRenderTargetView(*this, rtvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	case D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL:
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = m_TextureDesc.Format;
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D = D3D12_TEX2D_DSV();

		device->CreateDepthStencilView(*this, dsvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	}
}