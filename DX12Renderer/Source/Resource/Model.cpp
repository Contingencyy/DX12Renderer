#include "Pch.h"
#include "Resource/Model.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

Model::Model(const tinygltf::Model& glTFModel, const std::string& name)
	: m_Name(name)
{
	m_Buffers.resize(static_cast<std::size_t>(InputType::NUM_INPUT_TYPES));
	m_Textures.resize(static_cast<std::size_t>(TextureType::NUM_TEXTURE_TYPES));

	CreateBuffers(glTFModel);
	CreateTextures(glTFModel);
}

Model::~Model()
{
}

void Model::CreateBuffers(const tinygltf::Model& glTFModel)
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0", "NORMAL"};

	for (uint32_t i = 0; i < InputType::NUM_INPUT_TYPES - 1; ++i)
	{
		auto iter = glTFModel.meshes[0].primitives[0].attributes.find(attributeNames[i]);
		if (iter != glTFModel.meshes[0].primitives[0].attributes.end())
		{
			uint32_t attributeIndex = iter->second;
			auto& accessor = glTFModel.accessors[attributeIndex];
			auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
			auto& buffer = glTFModel.buffers[bufferView.buffer];

			const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
			m_Buffers[i] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);
		}
		else
		{
			ASSERT(false, "glTF model mesh does not contain attribute " + attributeNames[i]);
		}
	}

	// Load indices from glTF model
	uint32_t indiciesIndex = glTFModel.meshes[0].primitives[0].indices;
	auto& accessor = glTFModel.accessors[indiciesIndex];
	auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
	auto& buffer = glTFModel.buffers[bufferView.buffer];

	const unsigned char* dataPtr = &glTFModel.buffers[0].data[0] + bufferView.byteOffset;
	m_Buffers[InputType::INPUT_INDEX] = std::make_shared<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr);
}

void Model::CreateTextures(const tinygltf::Model& glTFModel)
{
	auto& material = glTFModel.materials[0];
	uint32_t albedoTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
	uint32_t albedoImageIndex = glTFModel.textures[albedoTextureIndex].source;

	m_Textures[TextureType::TEX_ALBEDO] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
		glTFModel.images[albedoImageIndex].width, glTFModel.images[albedoImageIndex].height), &glTFModel.images[albedoImageIndex].image[0]);

	uint32_t normalTextureIndex = material.normalTexture.index;
	uint32_t normalImageIndex = glTFModel.textures[normalTextureIndex].source;

	m_Textures[TextureType::TEX_NORMAL] = std::make_shared<Texture>(TextureDesc(TextureUsage::TEXTURE_USAGE_SHADER_RESOURCE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
		glTFModel.images[normalImageIndex].width, glTFModel.images[normalImageIndex].height), &glTFModel.images[normalImageIndex].image[0]);
}
