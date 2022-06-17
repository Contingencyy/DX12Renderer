#include "Pch.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/Texture.h"

DXGI_FORMAT DXGIFormatFromTextureFormat(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::TEXTURE_FORMAT_RGB10A2_UNORM:
		return DXGI_FORMAT_R10G10B10A2_UNORM;
	case TextureFormat::TEXTURE_FORMAT_DEPTH32:
		return DXGI_FORMAT_D32_FLOAT;
	}

	LOG_ERR("Format is not supported");
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

Texture::Texture(const TextureDesc& textureDesc, const void* data)
	: m_TextureDesc(textureDesc)
{
	Create();

	Buffer uploadBuffer(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, m_ByteSize));
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
		switch (m_TextureDesc.Usage)
		{
		case TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE:
		{
			m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			break;
		}
		case TextureUsage::TEXTURE_USAGE_RENDER_TARGET:
		{
			m_DescriptorAllocation = Application::Get().GetRenderer()->AllocateDescriptors(1, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
			break;
		}
		case TextureUsage::TEXTURE_USAGE_DEPTH:
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
	
	d3d12ResourceDesc.Format = DXGIFormatFromTextureFormat(m_TextureDesc.Format);

	switch (m_TextureDesc.Usage)
	{
	case TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE:
		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		Application::Get().GetRenderer()->GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_COMMON);
		break;
	case TextureUsage::TEXTURE_USAGE_RENDER_TARGET:
		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		Application::Get().GetRenderer()->GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET);
		break;
	case TextureUsage::TEXTURE_USAGE_DEPTH:
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = d3d12ResourceDesc.Format;
		clearValue.DepthStencil = { 1.0f, 0 };

		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		Application::Get().GetRenderer()->GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue);
		break;
	}

	m_ByteSize = GetRequiredIntermediateSize(m_d3d12Resource.Get(), 0, 1);
}

void Texture::CreateView()
{
	auto device = Application::Get().GetRenderer()->GetDevice();

	switch (m_TextureDesc.Usage)
	{
	case TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE:
	{
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = DXGIFormatFromTextureFormat(m_TextureDesc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

		device->CreateShaderResourceView(*this, srvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	case TextureUsage::TEXTURE_USAGE_RENDER_TARGET:
	{
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = DXGIFormatFromTextureFormat(m_TextureDesc.Format);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D = D3D12_TEX2D_RTV();

		device->CreateRenderTargetView(*this, rtvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	case TextureUsage::TEXTURE_USAGE_DEPTH:
	{
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = DXGIFormatFromTextureFormat(m_TextureDesc.Format);
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D = D3D12_TEX2D_DSV();

		device->CreateDepthStencilView(*this, dsvDesc, m_DescriptorAllocation.GetDescriptorHandle());
		break;
	}
	}
}
