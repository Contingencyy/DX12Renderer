#include "Pch.h"
#include "Graphics/Material.h"

Material::Material(const std::shared_ptr<Texture>& baseColorTexture, const std::shared_ptr<Texture>& normalTexture,
	const std::shared_ptr<Texture>& metallicRoughnessTexture, AlphaMode alphaMode)
	: m_BaseColorTexture(baseColorTexture), m_NormalTexture(normalTexture), m_MetallicRoughnessTexture(metallicRoughnessTexture), m_AlphaMode(alphaMode)
{
}
