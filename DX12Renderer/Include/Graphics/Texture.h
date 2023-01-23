#pragma once
#include "Graphics/Resource.h"
#include "Graphics/RenderAPI.h"

DXGI_FORMAT TextureFormatToDXGIFormat(TextureFormat format);
D3D12_RESOURCE_DIMENSION TextureDimensionToD3DDimension(TextureDimension dimension);
D3D12_SRV_DIMENSION TextureDimensionToD3DSRVDimension(TextureDimension dimension);
uint16_t CalculateTotalMipCount(uint32_t width, uint32_t height);

class Texture : public Resource
{
public:
	Texture() = default;
	Texture(const TextureDesc& textureDesc);
	virtual ~Texture();

	virtual bool IsValid() const;
	virtual bool IsCPUAccessible() const;
	virtual void Invalidate();

	void Resize(uint32_t width, uint32_t height);

	TextureDesc& GetTextureDesc() { return m_TextureDesc; }
	const TextureDesc& GetTextureDesc() const { return m_TextureDesc; }

protected:
	virtual void CreateD3D12Resource();
	virtual void AllocateDescriptors();
	virtual void CreateViews();

protected:
	TextureDesc m_TextureDesc = {};

};
