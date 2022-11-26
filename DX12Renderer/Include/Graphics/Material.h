#pragma once

class Texture;

enum AlphaMode : uint32_t
{
	OPAQUE = 0, TRANSPARENT = 1,
	NUM_ALPHA_MODES = 2
};

class Material
{
public:
	Material(const std::shared_ptr<Texture>& albedo, const std::shared_ptr<Texture>& normal, AlphaMode alphaMode);

	const std::shared_ptr<Texture>& GetAlbedoTexture() const { return m_Albedo; }
	const std::shared_ptr<Texture>& GetNormalTexture() const { return m_Normal; }
	AlphaMode GetAlphaMode() const { return m_AlphaMode; }

private:
	std::shared_ptr<Texture> m_Albedo;
	std::shared_ptr<Texture> m_Normal;

	AlphaMode m_AlphaMode;

};
