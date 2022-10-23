#include "Pch.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/RenderBackend.h"

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

	LOG_ERR("Texture format is not supported");
	return DXGI_FORMAT_R8G8B8A8_UNORM;
}

Texture::Texture(const std::string& name, const TextureDesc& textureDesc, const void* data)
	: m_TextureDesc(textureDesc)
{
	if (IsValid())
	{
		Create();
		CreateViews();
		SetName(name);

		Buffer uploadBuffer(m_Name + " - Upload buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 1, m_ByteSize));
		RenderBackend::CopyTexture(uploadBuffer, *this, data);
	}
}

Texture::Texture(const std::string& name, const TextureDesc& textureDesc)
	: m_TextureDesc(textureDesc)
{
	if (IsValid())
	{
		Create();
		CreateViews();
		SetName(name);
	}
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

bool Texture::IsValid() const
{
	return m_TextureDesc.Usage != TextureUsage::TEXTURE_USAGE_NONE && m_TextureDesc.Format != TextureFormat::TEXTURE_FORMAT_UNSPECIFIED;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetDescriptorHandle(DescriptorType type) const
{
	return m_DescriptorAllocations[type].GetCPUDescriptorHandle();
}

uint32_t Texture::GetDescriptorIndex(DescriptorType type) const
{
	return m_DescriptorAllocations[type].GetOffsetInDescriptorHeap();
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
	D3D12_RESOURCE_STATES initialState = D3D12_RESOURCE_STATE_COMMON;

	D3D12_CLEAR_VALUE clearValue = {};
	bool hasClearValue = false;

	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_READ)
	{
		// No special flags have to be set for SRVs
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_WRITE)
	{
		d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_RENDER_TARGET)
	{
		clearValue.Format = d3d12ResourceDesc.Format;
		//glm::vec4 defaultClearValue(0.2f, 0.2f, 0.2f, 1.0f);
		memcpy(clearValue.Color, &m_TextureDesc.ClearColor, sizeof(float) * 4);

		d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		initialState = D3D12_RESOURCE_STATE_RENDER_TARGET;
		hasClearValue = true;
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_DEPTH)
	{
		clearValue.Format = d3d12ResourceDesc.Format;
		clearValue.DepthStencil = { m_TextureDesc.ClearDepthStencil.x, static_cast<UINT8>(m_TextureDesc.ClearDepthStencil.y) };

		// TEMP to get D3D to shut up about different clear values during creation for shadow maps
		if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_READ)
			clearValue.DepthStencil = { 0.0f, 0 };

		d3d12ResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
		initialState = D3D12_RESOURCE_STATE_DEPTH_WRITE;
		hasClearValue = true;
	}

	RenderBackend::GetDevice()->CreateTexture(*this, d3d12ResourceDesc, initialState, hasClearValue ? &clearValue : nullptr);
	m_ByteSize = GetRequiredIntermediateSize(m_d3d12Resource.Get(), 0, 1);
}

void Texture::CreateViews()
{
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_READ)
	{
		auto& srv = m_DescriptorAllocations[DescriptorType::SRV];

		// Create shader resource view (read-only)
		if (srv.IsNull())
			srv = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		if (srvDesc.Format == DXGI_FORMAT_D32_FLOAT)
			srvDesc.Format = DXGI_FORMAT_R32_FLOAT;

		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = m_d3d12Resource->GetDesc().MipLevels;

		RenderBackend::GetDevice()->CreateShaderResourceView(*this, srvDesc, srv.GetCPUDescriptorHandle());
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_WRITE)
	{
		auto& uav = m_DescriptorAllocations[DescriptorType::UAV];

		// Create unordered access view (read/write)
		if (uav.IsNull())
			uav = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
		uavDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
		uavDesc.Buffer.FirstElement = 0;
		uavDesc.Buffer.NumElements = 1;
		uavDesc.Buffer.CounterOffsetInBytes = 0;

		RenderBackend::GetDevice()->CreateUnorderedAccessView(*this, uavDesc, uav.GetCPUDescriptorHandle());
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_RENDER_TARGET)
	{
		auto& rtv = m_DescriptorAllocations[DescriptorType::RTV];
		
		if (rtv.IsNull())
			rtv = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc = {};
		rtvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
		rtvDesc.Texture2D = D3D12_TEX2D_RTV();

		RenderBackend::GetDevice()->CreateRenderTargetView(*this, rtvDesc, rtv.GetCPUDescriptorHandle());
	}
	if (m_TextureDesc.Usage & TextureUsage::TEXTURE_USAGE_DEPTH)
	{
		auto& dsv = m_DescriptorAllocations[DescriptorType::DSV];

		if (dsv.IsNull())
			dsv = RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);

		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
		dsvDesc.Format = TextureFormatToDXGIFormat(m_TextureDesc.Format);
		dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
		dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
		dsvDesc.Texture2D = D3D12_TEX2D_DSV();

		RenderBackend::GetDevice()->CreateDepthStencilView(*this, dsvDesc, dsv.GetCPUDescriptorHandle());
	}
}
