#pragma once
#include <tinygltf/tiny_gltf.h>

class Mesh;
class Texture;

enum TextureType : uint32_t
{
	TEX_ALBEDO,
	TEX_NORMAL,
	NUM_TEXTURE_TYPES
};

class Model
{
public:
	Model(const tinygltf::Model& glTFModel, const std::string& name);
	~Model();

	const std::string& GetName() const { return m_Name; }
	std::shared_ptr<Mesh> GetMesh() const { return m_Mesh; }
	std::shared_ptr<Texture> GetTexture(TextureType type) const { return m_Textures[type]; }

private:
	void CreateTextures(const tinygltf::Model& glTFModel);

private:
	std::string m_Name = "";

	std::shared_ptr<Mesh> m_Mesh;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
