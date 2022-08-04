#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;

enum MeshBufferAttributeType : uint32_t
{
	ATTRIB_POSITION,
	ATTRIB_TEX_COORD,
	ATTRIB_NORMAL,
	ATTRIB_INDEX,
	NUM_ATTRIBUTE_TYPES
};

class Mesh
{
public:
	Mesh(const tinygltf::Model& glTFModel);
	~Mesh();

	std::shared_ptr<Buffer> GetBuffer(MeshBufferAttributeType type) const;

private:
	void CreateBuffers(const tinygltf::Model& glTFModel);

private:
	std::shared_ptr<Buffer> m_MeshBuffers[MeshBufferAttributeType::NUM_ATTRIBUTE_TYPES];

};
