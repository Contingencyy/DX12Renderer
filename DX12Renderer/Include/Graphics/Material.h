#pragma once

class Texture;

enum TransparencyMode : uint32_t
{
	OPAQUE = 0, TRANSPARENT = 1,
	NUM_ALPHA_MODES = 2
};

struct Material
{
public:
	Material() = default;

	std::shared_ptr<Texture> AlbedoTexture;
	std::shared_ptr<Texture> NormalTexture;
	std::shared_ptr<Texture> MetallicRoughnessTexture;

	float MetalnessFactor = 0.0f;
	float RoughnessFactor = 0.3f;

	TransparencyMode Transparency = TransparencyMode::OPAQUE;
	
};
