#include "Pch.h"
#include "Graphics/Mesh.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

Mesh::Mesh(const tinygltf::Model& glTFModel, const tinygltf::Primitive& glTFPrimitive, const std::string& name, std::size_t hash)
	: m_Name(name), m_Hash(hash)
{
	CreateBuffers(glTFModel, glTFPrimitive);
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

const std::string& Mesh::GetName() const
{
	return m_Name;
}

std::size_t Mesh::GetHash() const
{
	return m_Hash;
}

void Mesh::CreateBuffers(const tinygltf::Model& glTFModel, const tinygltf::Primitive& glTFPrimitive)
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0", "NORMAL" };
	uint32_t idx = 0;

	for (auto& attributeName : attributeNames)
	{
		auto iter = glTFPrimitive.attributes.find(attributeName);

		if (iter != glTFPrimitive.attributes.end())
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
	uint32_t indiciesIndex = glTFPrimitive.indices;
	auto& accessor = glTFModel.accessors[indiciesIndex];
	auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
	auto& buffer = glTFModel.buffers[bufferView.buffer];

	const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset + accessor.byteOffset;
	ASSERT((bufferView.byteOffset < buffer.data.size()), "Buffer view byte offset exceeded buffer total size");

	m_MeshBuffers[MeshBufferAttributeType::ATTRIB_INDEX] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);

	// Create textures for current mesh
	CreateTextures(glTFModel, glTFPrimitive.material);
}

void Mesh::CreateTextures(const tinygltf::Model& glTFModel, uint32_t matID)
{
	auto& material = glTFModel.materials[matID];
	int baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
	int normalTextureIndex = material.normalTexture.index;

	if (baseColorTextureIndex >= 0)
	{
		uint32_t baseColorImageIndex = glTFModel.textures[baseColorTextureIndex].source;

		m_Textures[MeshTextureType::TEX_BASE_COLOR] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			glTFModel.images[baseColorImageIndex].width, glTFModel.images[baseColorImageIndex].height), &glTFModel.images[baseColorImageIndex].image[0]);
	}
	else
	{
		uint32_t whiteImageData = 0xFFFFFFFF;
		m_Textures[MeshTextureType::TEX_BASE_COLOR] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			1, 1), &whiteImageData);
	}

	if (normalTextureIndex >= 0)
	{
		uint32_t normalImageIndex = glTFModel.textures[normalTextureIndex].source;

		m_Textures[MeshTextureType::TEX_NORMAL] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			glTFModel.images[normalImageIndex].width, glTFModel.images[normalImageIndex].height), &glTFModel.images[normalImageIndex].image[0]);
	}
	else
	{
		uint32_t whiteImageData = 0xFFFFFFFF;
		m_Textures[MeshTextureType::TEX_NORMAL] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
			1, 1), &whiteImageData);
	}
}
