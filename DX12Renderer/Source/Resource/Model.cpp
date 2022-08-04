#include "Pch.h"
#include "Resource/Model.h"
#include "Graphics/Mesh.h"
#include "Graphics/Texture.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

Model::Model(const tinygltf::Model& glTFModel, const std::string& name)
	: m_Name(name)
{
	m_Mesh = std::make_shared<Mesh>(glTFModel);

	m_Textures.resize(static_cast<std::size_t>(TextureType::NUM_TEXTURE_TYPES));
	CreateTextures(glTFModel);
}

Model::~Model()
{
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
