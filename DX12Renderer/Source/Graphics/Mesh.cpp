#include "Pch.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

Mesh::Mesh(const tinygltf::Model& glTFModel, uint32_t meshID)
{
	CreateBuffers(glTFModel, meshID);
}

Mesh::~Mesh()
{
}

std::shared_ptr<Buffer> Mesh::GetBuffer(MeshBufferAttributeType type) const
{
	return m_MeshBuffers[type];
}

std::shared_ptr<Texture> Mesh::GetTexture(MeshTextureType type) const
{
	return m_Textures[type];
}

void Mesh::CreateBuffers(const tinygltf::Model& glTFModel, uint32_t meshID)
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0", "NORMAL" };
	uint32_t idx = 0;

	for (auto& attributeName : attributeNames)
	{
		auto iter = glTFModel.meshes[0].primitives[meshID].attributes.find(attributeName);

		if (iter != glTFModel.meshes[0].primitives[meshID].attributes.end())
		{
			uint32_t attributeIndex = iter->second;
			auto& accessor = glTFModel.accessors[attributeIndex];
			auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
			auto& buffer = glTFModel.buffers[bufferView.buffer];

			const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
			ASSERT((bufferView.byteOffset < buffer.data.size()), "Buffer view byte offset exceeded buffer total size");

			m_MeshBuffers[idx] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);
		}
		else
		{
			ASSERT(false, "Mesh primitive does not contain attribute " + attributeName);
		}

		++idx;
	}

	// Load indices from glTF model
	uint32_t indiciesIndex = glTFModel.meshes[0].primitives[meshID].indices;
	auto& accessor = glTFModel.accessors[indiciesIndex];
	auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
	auto& buffer = glTFModel.buffers[bufferView.buffer];

	const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset + accessor.byteOffset;
	ASSERT((bufferView.byteOffset < buffer.data.size()), "Buffer view byte offset exceeded buffer total size");

	m_MeshBuffers[MeshBufferAttributeType::ATTRIB_INDEX] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);

	// Create textures for current mesh
	CreateTextures(glTFModel, glTFModel.meshes[0].primitives[meshID].material);
}

void Mesh::CreateTextures(const tinygltf::Model& glTFModel, uint32_t matID)
{
	auto& material = glTFModel.materials[matID];
	uint32_t albedoTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
	uint32_t albedoImageIndex = glTFModel.textures[albedoTextureIndex].source;

	m_Textures[MeshTextureType::TEX_ALBEDO] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
		glTFModel.images[albedoImageIndex].width, glTFModel.images[albedoImageIndex].height), &glTFModel.images[albedoImageIndex].image[0]);

	if (material.normalTexture.index >= 0)
	{
		uint32_t normalTextureIndex = material.normalTexture.index;
		uint32_t normalImageIndex = glTFModel.textures[normalTextureIndex].source;

		m_Textures[MeshTextureType::TEX_NORMAL] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			glTFModel.images[normalImageIndex].width, glTFModel.images[normalImageIndex].height), &glTFModel.images[normalImageIndex].image[0]);
	}
	else
	{
		m_Textures[MeshTextureType::TEX_NORMAL] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			glTFModel.images[albedoImageIndex].width, glTFModel.images[albedoImageIndex].height), &glTFModel.images[albedoImageIndex].image[0]);
	}
}
