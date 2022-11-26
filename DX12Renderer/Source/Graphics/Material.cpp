#include "Pch.h"
#include "Graphics/Material.h"

Material::Material(const std::shared_ptr<Texture>& albedo, const std::shared_ptr<Texture>& normal, AlphaMode alphaMode)
	: m_Albedo(albedo), m_Normal(normal), m_AlphaMode(alphaMode)
{
}
