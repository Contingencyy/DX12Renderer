#include "Pch.h"
#include "ResourceManager.h"
#include "ResourceLoader.h"
#include "Graphics/Texture.h"
#include "Graphics/Model.h"

ResourceManager::ResourceManager()
{
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::LoadTexture(const std::string& filepath, const std::string& name)
{
	ImageInfo imageInfo = ResourceLoader::LoadImage(filepath);

	TextureDesc textureDesc = {};
	textureDesc.Width = imageInfo.Width;
	textureDesc.Height = imageInfo.Height;
	m_Textures.insert(std::pair<std::string, std::shared_ptr<Texture>>(name, std::make_shared<Texture>(textureDesc)));

	delete imageInfo.Data;
}

void ResourceManager::LoadModel(const std::string& filepath, const std::string& name)
{
	const tinygltf::Model& glTFModel = ResourceLoader::LoadModel(filepath);
	m_Models.insert(std::pair<std::string, std::shared_ptr<Model>>(name, std::make_shared<Model>(glTFModel, name)));
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& name)
{
	return m_Textures.at(name);
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name)
{
	return m_Models.at(name);
}
