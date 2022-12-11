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
	Material(const std::shared_ptr<Texture>& baseColorTexture, const std::shared_ptr<Texture>& normalTexture,
		const std::shared_ptr<Texture>& metallicRougnessTexture, AlphaMode alphaMode);

	const std::shared_ptr<Texture>& GetBaseColorTexture() const { return m_BaseColorTexture; }
	const std::shared_ptr<Texture>& GetNormalTexture() const { return m_NormalTexture; }
	const std::shared_ptr<Texture>& GetMetallicRoughnessTexture() const { return m_MetallicRoughnessTexture; }
	AlphaMode GetAlphaMode() const { return m_AlphaMode; }

private:
	std::shared_ptr<Texture> m_BaseColorTexture;
	std::shared_ptr<Texture> m_NormalTexture;
	std::shared_ptr<Texture> m_MetallicRoughnessTexture;

	AlphaMode m_AlphaMode;

};
