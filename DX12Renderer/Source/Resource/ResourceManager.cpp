#include "Pch.h"
#include "Resource/ResourceManager.h"
#include "Resource/FileLoader.h"
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
	ImageInfo imageInfo = FileLoader::LoadImage(filepath);

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
	const tinygltf::Model& tinygltf = FileLoader::LoadGLTFModel(filepath);

	// Create all textures
	std::vector<std::vector<std::shared_ptr<Texture>>> textures;

	for (uint32_t matIndex = 0; matIndex < tinygltf.materials.size(); ++matIndex)
	{
		textures.emplace_back();
		textures[matIndex].reserve(2);

		auto& material = tinygltf.materials[matIndex];
		int baseColorTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
		int normalTextureIndex = material.normalTexture.index;

		if (baseColorTextureIndex >= 0)
		{
			uint32_t baseColorImageIndex = tinygltf.textures[baseColorTextureIndex].source;
			textures[matIndex].push_back(std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				tinygltf.images[baseColorImageIndex].width, tinygltf.images[baseColorImageIndex].height), &tinygltf.images[baseColorImageIndex].image[0]));
		}
		else
		{
			textures[matIndex].push_back(m_Textures.at("WhiteTexture"));
		}

		if (normalTextureIndex >= 0)
		{
			uint32_t normalImageIndex = tinygltf.textures[normalTextureIndex].source;
			textures[matIndex].push_back(std::make_shared<Texture>("Normal texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				tinygltf.images[normalImageIndex].width, tinygltf.images[normalImageIndex].height), &tinygltf.images[normalImageIndex].image[0]));
		}
		else
		{
			textures[matIndex].push_back(m_Textures.at("WhiteTexture"));
		}
	}

	std::size_t totalVertexCount = 0;
	std::size_t totalIndexCount = 0;
	std::size_t totalMeshCount = 0;

	// Loop through all meshes/primitives in GLTF to predetermine total amount of vertices/indices/meshes
	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			uint32_t vertexPosIndex = prim.attributes.find("POSITION")->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			totalVertexCount += vertexPosAccessor.count;

			uint32_t indicesIndex = prim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			totalIndexCount += indexAccessor.count;

			totalMeshCount++;
		}
	}

	struct Vertex
	{
		glm::vec3 Position;
		glm::vec2 TexCoord;
		glm::vec3 Normal;
	};

	std::vector<Vertex> vertices;
	std::vector<WORD> indices;
	
	vertices.reserve(totalVertexCount);
	indices.reserve(totalIndexCount);

	std::vector<std::shared_ptr<Mesh>> meshes;
	meshes.reserve(totalMeshCount);

	std::size_t currentStartVertex = 0;
	std::size_t currentStartIndex = 0;

	for (auto& mesh : tinygltf.meshes)
	{
		for (auto& prim : mesh.primitives)
		{
			currentStartVertex = vertices.size();
			currentStartIndex = indices.size();

			// Get vertex position data
			auto vertexPosAttrib = prim.attributes.find("POSITION");
			ASSERT(vertexPosAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute POSITION");

			uint32_t vertexPosIndex = vertexPosAttrib->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			const tinygltf::BufferView& vertexPosBufferView = tinygltf.bufferViews[vertexPosAccessor.bufferView];
			const tinygltf::Buffer& vertexPosBuffer = tinygltf.buffers[vertexPosBufferView.buffer];

			const float* pVertexPosData = reinterpret_cast<const float*>(&vertexPosBuffer.data[0] + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset);
			ASSERT(vertexPosAccessor.count * vertexPosAccessor.ByteStride(vertexPosBufferView) + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset <= vertexPosBuffer.data.size(),
				"Byte offset for vertex attribute POSITION exceeded total buffer size");

			// Get vertex tex coordinates data
			auto vertexTexCoordAttrib = prim.attributes.find("TEXCOORD_0");
			ASSERT(vertexTexCoordAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute TEXCOORD_0");

			uint32_t vertexTexCoordIndex = vertexTexCoordAttrib->second;
			const tinygltf::Accessor& vertexTexCoordAccessor = tinygltf.accessors[vertexTexCoordIndex];
			const tinygltf::BufferView& vertexTexCoordBufferView = tinygltf.bufferViews[vertexTexCoordAccessor.bufferView];
			const tinygltf::Buffer& vertexTexCoordBuffer = tinygltf.buffers[vertexTexCoordBufferView.buffer];

			const float* pVertexTexCoordData = reinterpret_cast<const float*>(&vertexTexCoordBuffer.data[0] + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset);
			ASSERT(vertexTexCoordAccessor.count * vertexTexCoordAccessor.ByteStride(vertexTexCoordBufferView) + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset <= vertexTexCoordBuffer.data.size(),
				"Byte offset for vertex attribute TEXCOORD_0 exceeded total buffer size");

			// Get vertex normal data
			auto vertexNormalAttrib = prim.attributes.find("NORMAL");
			ASSERT(vertexNormalAttrib != prim.attributes.end(), "GLTF primitive does not contain vertex attribute NORMAL");

			uint32_t vertexNormalIndex = vertexNormalAttrib->second;
			const tinygltf::Accessor& vertexNormalAccessor = tinygltf.accessors[vertexNormalIndex];
			const tinygltf::BufferView& vertexNormalBufferView = tinygltf.bufferViews[vertexNormalAccessor.bufferView];
			const tinygltf::Buffer& vertexNormalBuffer = tinygltf.buffers[vertexNormalBufferView.buffer];

			const float* pVertexNormalData = reinterpret_cast<const float*>(&vertexNormalBuffer.data[0] + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset);
			ASSERT(vertexNormalAccessor.count * vertexNormalAccessor.ByteStride(vertexNormalBufferView) + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset <= vertexNormalBuffer.data.size(),
				"Byte offset for vertex attribute NORMAL exceeded total buffer size");

			// Set attribute indices
			uint32_t posIndex = 0;
			uint32_t texCoordIndex = 0;
			uint32_t normalIndex = 0;

			// Construct a vertex from the primitive attributes data and add it to all vertices
			for (uint32_t i = 0; i < vertexPosAccessor.count; ++i)
			{
				Vertex v = {};
				v.Position = glm::vec3(pVertexPosData[posIndex], pVertexPosData[posIndex + 1], pVertexPosData[posIndex + 2]);
				v.TexCoord = glm::vec2(pVertexTexCoordData[texCoordIndex], pVertexTexCoordData[texCoordIndex + 1]);
				v.Normal = glm::vec3(pVertexNormalData[normalIndex], pVertexNormalData[normalIndex + 1], pVertexNormalData[normalIndex + 2]);

				vertices.push_back(v);

				posIndex += 3;
				texCoordIndex += 2;
				normalIndex += 3;
			}

			// Set the min and max bounds for the current primitive/mesh
			glm::vec3 minBounds = glm::vec3(vertexPosAccessor.minValues[0], vertexPosAccessor.minValues[1], vertexPosAccessor.minValues[2]);
			glm::vec3 maxBounds = glm::vec3(vertexPosAccessor.maxValues[0], vertexPosAccessor.maxValues[1], vertexPosAccessor.maxValues[2]);

			// Get index data
			uint32_t indicesIndex = prim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			const tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = tinygltf.buffers[indexBufferView.buffer];

			const WORD* pIndexData = reinterpret_cast<const WORD*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);
			ASSERT(indexAccessor.count * indexAccessor.ByteStride(indexBufferView) + indexBufferView.byteOffset + indexAccessor.byteOffset,
				"Byte offset for indices exceeded total buffer size");

			// Get indices for current primitive/mesh and add it to all indices
			std::size_t numIndices = 0;

			for (uint32_t i = 0; i < indexAccessor.count; ++i)
			{
				indices.push_back(pIndexData[i]);
				numIndices++;
			}

			// Meshes need to know their byte offset in both the vertex and index buffer
			std::string meshName = mesh.name.empty() ? name + std::to_string(meshes.size()) : mesh.name + std::to_string(meshes.size());
			std::size_t meshHash = std::hash<std::string>{}(filepath + std::to_string(meshes.size()));
			meshes.push_back(std::make_shared<Mesh>(textures[prim.material], currentStartVertex, currentStartIndex, numIndices, minBounds, maxBounds, meshName, meshHash));
		}
	}

	std::shared_ptr<Buffer> modelVertexBuffer = std::make_shared<Buffer>(name + " vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, vertices.size(), sizeof(Vertex)), &vertices[0]);
	std::shared_ptr<Buffer> modelIndexBuffer = std::make_shared<Buffer>(name + " index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, indices.size(), sizeof(WORD)), &indices[0]);

	for (auto& mesh : meshes)
	{
		mesh->SetVertexBuffer(modelVertexBuffer);
		mesh->SetIndexBuffer(modelIndexBuffer);
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
