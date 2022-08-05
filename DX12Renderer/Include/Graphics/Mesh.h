#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;

enum MeshBufferAttributeType : uint32_t
{
	ATTRIB_POSITION,
	ATTRIB_TEX_COORD,
	ATTRIB_NORMAL,
	ATTRIB_INDEX,
	NUM_ATTRIBUTE_TYPES
};

enum MeshTextureType : uint32_t
{
	TEX_ALBEDO,
	TEX_NORMAL,
	NUM_TEXTURE_TYPES
};

class Mesh
{
public:
	Mesh(const tinygltf::Model& glTFModel, uint32_t meshID);
	~Mesh();

	std::shared_ptr<Buffer> GetBuffer(MeshBufferAttributeType type) const;
	std::shared_ptr<Texture> GetTexture(MeshTextureType type) const;

private:
	void CreateBuffers(const tinygltf::Model& glTFModel, uint32_t meshID);
	void CreateTextures(const tinygltf::Model& glTFModel, uint32_t matID);

private:
	std::shared_ptr<Buffer> m_MeshBuffers[MeshBufferAttributeType::NUM_ATTRIBUTE_TYPES];
	std::shared_ptr<Texture> m_Textures[MeshTextureType::NUM_TEXTURE_TYPES];

};
