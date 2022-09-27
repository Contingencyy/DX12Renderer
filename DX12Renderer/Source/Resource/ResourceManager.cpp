#include "Pch.h"
#include "Resource/ResourceManager.h"
#include "Resource/ResourceLoader.h"
#include "Resource/Model.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Mesh.h"

ResourceManager::ResourceManager()
{
	uint32_t whiteTextureData = 0xFFFFFFFF;
	m_Textures.insert(std::pair<std::string, std::shared_ptr<Texture>>("WhiteTexture", std::make_shared<Texture>("WhiteTexture", TextureDesc(
		TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM, 1, 1), &whiteTextureData)));
}

ResourceManager::~ResourceManager()
{
}

void ResourceManager::LoadTexture(const std::string& filepath, const std::string& name)
{
	ImageInfo imageInfo = ResourceLoader::LoadImage(filepath);

	TextureDesc textureDesc = {};
	textureDesc.Usage = TextureUsage::TEXTURE_USAGE_READ;
	textureDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
	textureDesc.Width = imageInfo.Width;
	textureDesc.Height = imageInfo.Height;
	m_Textures.insert(std::pair<std::string, std::shared_ptr<Texture>>(name, std::make_shared<Texture>(name, textureDesc)));

	delete imageInfo.Data;

	LOG_INFO("[ResourceManager] Loaded texture: " + filepath);
}

void ResourceManager::LoadModel(const std::string& filepath, const std::string& name)
{
	const tinygltf::Model& glTFModel = ResourceLoader::LoadGLTFModel(filepath);

	// Create all textures
	std::vector<std::vector<std::shared_ptr<Texture>>> textures;

	for (uint32_t matIndex = 0; matIndex < glTFModel.materials.size(); ++matIndex)
	{
		textures.emplace_back();
		textures[matIndex].reserve(2);

		auto& material = glTFModel.materials[matIndex];
		int baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
		int normalTextureIndex = material.normalTexture.index;

		if (baseColorTextureIndex >= 0)
		{
			uint32_t baseColorImageIndex = glTFModel.textures[baseColorTextureIndex].source;
			textures[matIndex].push_back(std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				glTFModel.images[baseColorImageIndex].width, glTFModel.images[baseColorImageIndex].height), &glTFModel.images[baseColorImageIndex].image[0]));
		}
		else
		{
			uint32_t whiteImageData = 0xFFFFFFFF;
			textures[matIndex].push_back(m_Textures.at("WhiteTexture"));
		}

		if (normalTextureIndex >= 0)
		{
			uint32_t normalImageIndex = glTFModel.textures[normalTextureIndex].source;
			textures[matIndex].push_back(std::make_shared<Texture>("Normal texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				glTFModel.images[normalImageIndex].width, glTFModel.images[normalImageIndex].height), &glTFModel.images[normalImageIndex].image[0]));
		}
		else
		{
			uint32_t whiteImageData = 0xFFFFFFFF;
			textures[matIndex].push_back(m_Textures.at("WhiteTexture"));
		}
	}

	// Create all meshes
	std::vector<std::shared_ptr<Mesh>> meshes;
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0", "NORMAL" };

	for (auto& mesh : glTFModel.meshes)
	{
		for (auto& primitive : mesh.primitives)
		{
			std::vector<std::shared_ptr<Buffer>> buffers;
			buffers.reserve(4);

			// Min/max bounds for mesh
			glm::vec3 minBounds(0.0f);
			glm::vec3 maxBounds(0.0f);

			// Loop through all needed attributes to load them and create buffers with their data
			for (auto& attributeName : attributeNames)
			{
				auto attribIter = primitive.attributes.find(attributeName);

				if (attribIter != primitive.attributes.end())
				{
					uint32_t attribIndex = attribIter->second;

					auto& accessor = glTFModel.accessors[attribIndex];
					auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
					auto& buffer = glTFModel.buffers[bufferView.buffer];

					const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
					ASSERT((bufferView.byteOffset < buffer.data.size()), "Buffer view byte offset exceeded buffer total size");

					buffers.push_back(std::make_shared<Buffer>(name + " vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr));

					if (attributeName == "POSITION")
					{
						minBounds = glm::vec3(accessor.minValues[0], accessor.minValues[1], accessor.minValues[2]);
						maxBounds = glm::vec3(accessor.maxValues[0], accessor.maxValues[1], accessor.maxValues[2]);
					}
				}
				else
				{
					ASSERT(false, "Mesh primitive does not contain attribute " + attributeName);
				}
			}

			// Create index buffer
			uint32_t indicesIndex = primitive.indices;

			auto& accessor = glTFModel.accessors[indicesIndex];
			auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
			auto& buffer = glTFModel.buffers[bufferView.buffer];

			const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset + accessor.byteOffset;
			ASSERT((bufferView.byteOffset < buffer.data.size()), "Buffer view byte offset exceeded buffer total size");

			buffers.push_back(std::make_shared<Buffer>(name + " index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, accessor.count, accessor.ByteStride(bufferView)), dataPtr));

			// Create the mesh with the buffers/textures, and additional data like min/max bounds, name, and hash
			// TODO: Name and hash generation
			meshes.push_back(std::make_shared<Mesh>(buffers, textures[primitive.material], minBounds, maxBounds, "Unnamed", meshes.size()));
		}
	}

	// Create model containing all of the meshes
	m_Models.insert(std::pair<std::string, std::shared_ptr<Model>>(name, std::make_shared<Model>(meshes, name)));
	LOG_INFO("[ResourceManager] Loaded model: " + filepath);
}

std::shared_ptr<Texture> ResourceManager::GetTexture(const std::string& name)
{
	ASSERT(m_Textures.find(name) != m_Textures.end(), "Texture does not exist");
	return m_Textures.at(name);
}

std::shared_ptr<Model> ResourceManager::GetModel(const std::string& name)
{
	ASSERT(m_Models.find(name) != m_Models.end(), "Model does not exist");
	return m_Models.at(name);
}
