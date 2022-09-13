#include "Pch.h"
#include "Graphics/Device.h"
#include "Graphics/Texture.h"
#include "Graphics/RenderBackend.h"

DXGI_FORMAT TextureFormatToDXGIFormat(TextureFormat format)
{
	switch (format)
	{
	case TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM:
		return DXGI_FORMAT_R8G8B8A8_UNORM;
	case TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT:
		return DXGI_FORMAT_R16G16B16A16_FLOAT;
	case TextureFormat::TEXTURE_FORMAT_DEPTH32:
		return DXGI_FORMAT_D32_FLOAT;
	}

	LOG_ERR("Format is not supported");
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

Texture::Texture(const std::string& name, const TextureDesc& textureDesc, const void* data)
	: m_TextureDesc(textureDesc)
{
	Create();
	CreateViews();
	SetName(name);

	Buffer uploadBuffer(m_Name + " - Upload buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, m_ByteSize));
	RenderBackend::Get().CopyTexture(uploadBuffer, *this, data);
}

Texture::Texture(const std::string& name, const TextureDesc& textureDesc)
	: m_TextureDesc(textureDesc)
{
	Create();
	CreateViews();
	SetName(name);
}

Texture::~Texture()
{
}

void Texture::Resize(uint32_t width, uint32_t height)
{
	m_TextureDesc.Width = width;
	m_TextureDesc.Height = height;

	Create();
	CreateViews();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetRenderTargetDepthStencilView()
{
	return m_RenderTargetDepthStencilDescriptor.GetCPUDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView()
{
	return m_ShaderResourceViewDescriptor.GetCPUDescriptorHandle();
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView()
{
	return m_UnorderedAccessViewDescriptor.GetCPUDescriptorHandle();
}

uint32_t Texture::GetSRVIndex() const
{
	return m_ShaderResourceViewDescriptor.GetDescriptorHeapOffset();
}

uint32_t Texture::GetUAVIndex() const
{
	return m_UnorderedAccessViewDescriptor.GetDescriptorHeapOffset();
}

void Texture::SetName(const std::string& name)
{
	m_Name = name;
	m_d3d12Resource->SetName(StringHelper::StringToWString(name).c_str());
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
	
	d3d12ResourceDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);

	switch (m_TextureDesc.Usage)
	{
	case TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE:
		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		RenderBackend::Get().GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_COMMON);
		break;
	case TextureUsage::TEXTURE_USAGE_RENDER_TARGET:
	case TextureUsage::TEXTURE_USAGE_BACK_BUFFER:
		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		RenderBackend::Get().GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_RENDER_TARGET);
		break;
	case TextureUsage::TEXTURE_USAGE_DEPTH:
		D3D12_CLEAR_VALUE clearValue = {};
		clearValue.Format = d3d12ResourceDesc.Format;
		clearValue.DepthStencil = { 1.0f, 0 };

		d3d12ResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		RenderBackend::Get().GetDevice()->CreateTexture(*this, d3d12ResourceDesc, D3D12_RESOURCE_STATE_DEPTH_WRITE, &clearValue);
		break;
	}

	m_ByteSize = GetRequiredIntermediateSize(m_d3d12Resource.Get(), 0, 1);
}

void Texture::CreateViews()
{
	// Create RTV/DSV based on texture usage
	switch (m_TextureDesc.Usage)
	{
	case TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE:
	{
		// Create shader resource view (read-only)
		if (m_ShaderResourceViewDescriptor.IsNull())
			m_ShaderResourceViewDescriptor = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

		RenderBackend::Get().GetDevice()->CreateShaderResourceView(*this, srvDesc, m_ShaderResourceViewDescriptor.GetCPUDescriptorHandle());

		// Create unordered access view (read/write)
		//if (m_UnorderedAccessViewDescriptor.IsNull())
		//	  m_UnorderedAccessViewDescriptor = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		//D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		//uavDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		//uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		///*uavDesc.Buffer.FirstElement = 0;
		//uavDesc.Buffer.NumElements = 1;
		//uavDesc.Buffer.CounterOffsetInBytes = 0;*/

		//RenderBackend::Get().GetDevice()->CreateUnorderedAccessView(*this, uavDesc, m_UnorderedAccessViewDescriptor.GetCPUDescriptorHandle());
		break;
	}
	case TextureUsage::TEXTURE_USAGE_RENDER_TARGET:
	{
		// Create shader resource view (read-only)
		if (m_ShaderResourceViewDescriptor.IsNull())
			m_ShaderResourceViewDescriptor = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

		RenderBackend::Get().GetDevice()->CreateShaderResourceView(*this, srvDesc, m_ShaderResourceViewDescriptor.GetCPUDescriptorHandle());
		[[fallthrough]];
	}
	case TextureUsage::TEXTURE_USAGE_BACK_BUFFER:
	{
		if (m_RenderTargetDepthStencilDescriptor.IsNull())
			m_RenderTargetDepthStencilDescriptor = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D = D3D12_TEX2D_RTV();

		RenderBackend::Get().GetDevice()->CreateRenderTargetView(*this, rtvDesc, m_RenderTargetDepthStencilDescriptor.GetCPUDescriptorHandle());
		break;
	}
	case TextureUsage::TEXTURE_USAGE_DEPTH:
	{
		if (m_RenderTargetDepthStencilDescriptor.IsNull())
			m_RenderTargetDepthStencilDescriptor = RenderBackend::Get().AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D = D3D12_TEX2D_DSV();

		RenderBackend::Get().GetDevice()->CreateDepthStencilView(*this, dsvDesc, m_RenderTargetDepthStencilDescriptor.GetCPUDescriptorHandle());
		break;
	}
	}
}
