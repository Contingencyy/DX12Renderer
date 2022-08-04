#include "Pch.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffer.h"

Mesh::Mesh(const tinygltf::Model& glTFModel)
{
	CreateBuffers(glTFModel);
}

Mesh::~Mesh()
{
}

std::shared_ptr<Buffer> Mesh::GetBuffer(MeshBufferAttributeType type) const
{
	return m_MeshBuffers[type];
}

void Mesh::CreateBuffers(const tinygltf::Model& glTFModel)
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0", "NORMAL" };
	uint32_t idx = 0;

	for (auto& attributeName : attributeNames)
	{
		auto iter = glTFModel.meshes[0].primitives[0].attributes.find(attributeName);
		if (iter != glTFModel.meshes[0].primitives[0].attributes.end())
		{
			uint32_t attributeIndex = iter->second;
			auto& accessor = glTFModel.accessors[attributeIndex];
			auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
			auto& buffer = glTFModel.buffers[bufferView.buffer];

			const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
			m_MeshBuffers[idx] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);
		}
		else
		{
			ASSERT(false, "glTF model mesh does not contain attribute " + attributeName);
		}

		++idx;
	}

	// Load indices from glTF model
	uint32_t indiciesIndex = glTFModel.meshes[0].primitives[0].indices;
	auto& accessor = glTFModel.accessors[indiciesIndex];
	auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
	auto& buffer = glTFModel.buffers[bufferView.buffer];

	const unsigned char* dataPtr = &glTFModel.buffers[0].data[0] + bufferView.byteOffset;
	m_MeshBuffers[MeshBufferAttributeType::ATTRIB_INDEX] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);
}
