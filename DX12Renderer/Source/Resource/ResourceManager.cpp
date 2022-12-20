#include "Pch.h"
#include "Resource/ResourceManager.h"
#include "Resource/FileLoader.h"
#include "Resource/Model.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Mesh.h"
#include "Graphics/Material.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};

ResourceManager::ResourceManager()
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
	std::vector<Material> materials;
	materials.reserve(tinygltf.materials.size());

	for (uint32_t matIndex = 0; matIndex < tinygltf.materials.size(); ++matIndex)
	{
		auto& gltfMaterial = tinygltf.materials[matIndex];

		int albedoTextureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
		int normalTextureIndex = gltfMaterial.normalTexture.index;
		int metallicRoughnessTextureIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;

		std::shared_ptr<Texture> albedoTexture;
		std::shared_ptr<Texture> normalTexture;
		std::shared_ptr<Texture> metallicRoughnessTexture;
		TransparencyMode transparency = gltfMaterial.alphaMode.compare("OPAQUE") == 0 ? TransparencyMode::OPAQUE : TransparencyMode::TRANSPARENT;

		if (albedoTextureIndex >= 0)
		{
			uint32_t albedoImageIndex = tinygltf.textures[albedoTextureIndex].source;
			albedoTexture = std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_SRGB,
				TextureDimension::TEXTURE_DIMENSION_2D, tinygltf.images[albedoImageIndex].width, tinygltf.images[albedoImageIndex].height,
				CalculateTotalMipCount(tinygltf.images[albedoImageIndex].width,	tinygltf.images[albedoImageIndex].height)), &tinygltf.images[albedoImageIndex].image[0]);
		}
		else
		{
			albedoTexture = std::make_shared<Texture>("Albedo texture", TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
				TextureDimension::TEXTURE_DIMENSION_UNSPECIFIED, 1, 1));
		}

		if (normalTextureIndex >= 0)
		{
			uint32_t normalImageIndex = tinygltf.textures[normalTextureIndex].source;
			normalTexture = std::make_shared<Texture>("Normal texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				TextureDimension::TEXTURE_DIMENSION_2D, tinygltf.images[normalImageIndex].width, tinygltf.images[normalImageIndex].height,
				CalculateTotalMipCount(tinygltf.images[normalImageIndex].width, tinygltf.images[normalImageIndex].height)), &tinygltf.images[normalImageIndex].image[0]);
		}
		else
		{
			normalTexture = std::make_shared<Texture>("Normal texture", TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
				TextureDimension::TEXTURE_DIMENSION_UNSPECIFIED, 1, 1));
		}

		if (metallicRoughnessTextureIndex >= 0)
		{
			uint32_t metallicRoughnessImageIndex = tinygltf.textures[metallicRoughnessTextureIndex].source;
			metallicRoughnessTexture = std::make_shared<Texture>("Metallic roughness texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
				TextureDimension::TEXTURE_DIMENSION_2D, tinygltf.images[metallicRoughnessImageIndex].width, tinygltf.images[metallicRoughnessImageIndex].height,
				CalculateTotalMipCount(tinygltf.images[metallicRoughnessImageIndex].width, tinygltf.images[metallicRoughnessImageIndex].height)), &tinygltf.images[metallicRoughnessImageIndex].image[0]);
		}
		else
		{
			metallicRoughnessTexture = std::make_shared<Texture>("Metallic roughness texture", TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
				TextureDimension::TEXTURE_DIMENSION_UNSPECIFIED, 1, 1));
		}

		Material material = {};
		material.AlbedoTexture = albedoTexture;
		material.NormalTexture = normalTexture;
		material.MetallicRoughnessTexture = metallicRoughnessTexture;
		material.MetalnessFactor = gltfMaterial.pbrMetallicRoughness.metallicFactor;
		material.RoughnessFactor = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
		material.Transparency = transparency;

		materials.emplace_back(material);
	}

	std::size_t totalVertexCount = 0;
	std::size_t totalIndexCount = 0;

	// Loop through all meshes/primitives in GLTF to predetermine total amount of vertices/indices/meshes
	for (auto& gltfMesh : tinygltf.meshes)
	{
		for (auto& prim : gltfMesh.primitives)
		{
			uint32_t vertexPosIndex = prim.attributes.find("POSITION")->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			totalVertexCount += vertexPosAccessor.count;

			uint32_t indicesIndex = prim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			totalIndexCount += indexAccessor.count;
		}
	}

	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;
	
	vertices.resize(totalVertexCount);
	indices.resize(totalIndexCount);

	std::vector<std::shared_ptr<Mesh>> meshes;
	meshes.reserve(tinygltf.meshes.size());

	std::size_t currentStartVertex = 0;
	std::size_t currentStartIndex = 0;

	for (auto& gltfMesh : tinygltf.meshes)
	{
		std::vector<MeshPrimitive> meshPrimitives;
		meshPrimitives.reserve(gltfMesh.primitives.size());

		for (auto& gltfPrim : gltfMesh.primitives)
		{
			// Get vertex position data
			auto vertexPosAttrib = gltfPrim.attributes.find("POSITION");
			ASSERT(vertexPosAttrib != gltfPrim.attributes.end(), "GLTF primitive does not contain vertex attribute POSITION");

			uint32_t vertexPosIndex = vertexPosAttrib->second;
			const tinygltf::Accessor& vertexPosAccessor = tinygltf.accessors[vertexPosIndex];
			const tinygltf::BufferView& vertexPosBufferView = tinygltf.bufferViews[vertexPosAccessor.bufferView];
			const tinygltf::Buffer& vertexPosBuffer = tinygltf.buffers[vertexPosBufferView.buffer];

			const glm::vec3* pVertexPosData = reinterpret_cast<const glm::vec3*>(&vertexPosBuffer.data[0] + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset);
			ASSERT(vertexPosAccessor.count * vertexPosAccessor.ByteStride(vertexPosBufferView) + vertexPosBufferView.byteOffset + vertexPosAccessor.byteOffset <= vertexPosBuffer.data.size(),
				"Byte offset for vertex attribute POSITION exceeded total buffer size");

			// Get vertex tex coordinates data
			auto vertexTexCoordAttrib = gltfPrim.attributes.find("TEXCOORD_0");
			ASSERT(vertexTexCoordAttrib != gltfPrim.attributes.end(), "GLTF primitive does not contain vertex attribute TEXCOORD_0");

			uint32_t vertexTexCoordIndex = vertexTexCoordAttrib->second;
			const tinygltf::Accessor& vertexTexCoordAccessor = tinygltf.accessors[vertexTexCoordIndex];
			const tinygltf::BufferView& vertexTexCoordBufferView = tinygltf.bufferViews[vertexTexCoordAccessor.bufferView];
			const tinygltf::Buffer& vertexTexCoordBuffer = tinygltf.buffers[vertexTexCoordBufferView.buffer];

			const glm::vec2* pVertexTexCoordData = reinterpret_cast<const glm::vec2*>(&vertexTexCoordBuffer.data[0] + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset);
			ASSERT(vertexTexCoordAccessor.count * vertexTexCoordAccessor.ByteStride(vertexTexCoordBufferView) + vertexTexCoordBufferView.byteOffset + vertexTexCoordAccessor.byteOffset <= vertexTexCoordBuffer.data.size(),
				"Byte offset for vertex attribute TEXCOORD_0 exceeded total buffer size");

			// Get vertex normal data
			auto vertexNormalAttrib = gltfPrim.attributes.find("NORMAL");
			ASSERT(vertexNormalAttrib != gltfPrim.attributes.end(), "GLTF primitive does not contain vertex attribute NORMAL");

			uint32_t vertexNormalIndex = vertexNormalAttrib->second;
			const tinygltf::Accessor& vertexNormalAccessor = tinygltf.accessors[vertexNormalIndex];
			const tinygltf::BufferView& vertexNormalBufferView = tinygltf.bufferViews[vertexNormalAccessor.bufferView];
			const tinygltf::Buffer& vertexNormalBuffer = tinygltf.buffers[vertexNormalBufferView.buffer];

			const glm::vec3* pVertexNormalData = reinterpret_cast<const glm::vec3*>(&vertexNormalBuffer.data[0] + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset);
			ASSERT(vertexNormalAccessor.count * vertexNormalAccessor.ByteStride(vertexNormalBufferView) + vertexNormalBufferView.byteOffset + vertexNormalAccessor.byteOffset <= vertexNormalBuffer.data.size(),
				"Byte offset for vertex attribute NORMAL exceeded total buffer size");

			// Construct a vertex from the primitive attributes data and add it to all vertices
			for (std::size_t i = currentStartVertex; i < currentStartVertex + vertexPosAccessor.count; ++i)
			{
				Vertex& v = vertices[i];
				v.Position = *pVertexPosData++;
				v.TexCoord = *pVertexTexCoordData++;
				v.Normal = *pVertexNormalData++;
			}

			// Get the vertex tangents/bitangents
			auto vertexTangentAttrib = gltfPrim.attributes.find("TANGENT");
			if (vertexTangentAttrib != gltfPrim.attributes.end())
			{
				uint32_t vertexTangentIndex = vertexTangentAttrib->second;
				const tinygltf::Accessor& vertexTangentAccessor = tinygltf.accessors[vertexTangentIndex];
				const tinygltf::BufferView& vertexTangentBufferView = tinygltf.bufferViews[vertexTangentAccessor.bufferView];
				const tinygltf::Buffer& vertexTangentBuffer = tinygltf.buffers[vertexTangentBufferView.buffer];

				const glm::vec4* pVertexTangentData = reinterpret_cast<const glm::vec4*>(&vertexTangentBuffer.data[0] + vertexTangentBufferView.byteOffset + vertexTangentAccessor.byteOffset);
				ASSERT(vertexTangentAccessor.count * vertexTangentAccessor.ByteStride(vertexTangentBufferView) + vertexTangentBufferView.byteOffset + vertexTangentAccessor.byteOffset <= vertexTangentBuffer.data.size(),
					"Byte offset for vertex attribute TANGENT exceeded total buffer size");

				for (std::size_t i = currentStartVertex; i < currentStartVertex + vertexTangentAccessor.count; ++i)
				{
					Vertex& vert0 = vertices[i];

					vert0.Tangent = glm::normalize(*pVertexTangentData);
					vert0.Bitangent = glm::normalize(glm::cross(vert0.Normal, vert0.Tangent) * pVertexTangentData->w);

					pVertexTangentData++;
				}
			}
			else
			{
				for (std::size_t i = currentStartVertex; i < currentStartVertex + vertexPosAccessor.count; i += 3)
				{
					Vertex& vert0 = vertices[i];
					Vertex& vert1 = vertices[i + 1];
					Vertex& vert2 = vertices[i + 2];

					CalculateVertexTangents(vert0, vert1, vert2);
				}
			}

			// Set the min and max bounds for the current primitive/mesh
			glm::vec3 minBounds = glm::vec3(vertexPosAccessor.minValues[0], vertexPosAccessor.minValues[1], vertexPosAccessor.minValues[2]);
			glm::vec3 maxBounds = glm::vec3(vertexPosAccessor.maxValues[0], vertexPosAccessor.maxValues[1], vertexPosAccessor.maxValues[2]);

			// Get index data
			uint32_t indicesIndex = gltfPrim.indices;
			const tinygltf::Accessor& indexAccessor = tinygltf.accessors[indicesIndex];
			const tinygltf::BufferView& indexBufferView = tinygltf.bufferViews[indexAccessor.bufferView];
			const tinygltf::Buffer& indexBuffer = tinygltf.buffers[indexBufferView.buffer];
			std::size_t indicesByteSize = indexAccessor.componentType == 5123 ? 2 : 4;

			ASSERT(indexAccessor.count * indexAccessor.ByteStride(indexBufferView) + indexBufferView.byteOffset + indexAccessor.byteOffset,
				"Byte offset for indices exceeded total buffer size");

			// Get indices for current primitive/mesh and add it to all indices
			if (indicesByteSize == 2)
			{
				const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);

				for (std::size_t i = currentStartIndex; i < currentStartIndex + indexAccessor.count; ++i)
				{
					indices[i] = static_cast<uint32_t>(*pIndexData++);
				}
			}
			else if (indicesByteSize == 4)
			{
				const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);

				for (std::size_t i = currentStartIndex; i < currentStartIndex + indexAccessor.count; ++i)
				{
					indices[i] = *pIndexData++;
				}
			}

			MeshPrimitive meshPrimitive = {};
			meshPrimitive.Material = materials[gltfPrim.material];
			meshPrimitive.BB.Min = static_cast<glm::vec3>(minBounds);
			meshPrimitive.BB.Max = static_cast<glm::vec3>(maxBounds);
			meshPrimitive.VerticesStart = currentStartVertex;
			meshPrimitive.IndicesStart = currentStartIndex;
			meshPrimitive.NumIndices = indexAccessor.count;
			meshPrimitive.Name = gltfMesh.name + std::to_string(meshPrimitives.size());

			meshPrimitives.push_back(meshPrimitive);

			currentStartVertex += vertexPosAccessor.count;
			currentStartIndex += indexAccessor.count;
		}

		// Meshes need to know their byte offset in both the vertex and index buffer
		Mesh mesh = {};
		mesh.Primitives = meshPrimitives;
		mesh.Name = gltfMesh.name.empty() ? name + std::to_string(meshes.size()) : gltfMesh.name + std::to_string(meshes.size());
		mesh.Hash = std::hash<std::string>{}(filepath + std::to_string(meshes.size()));

		std::shared_ptr<Buffer> vertexBuffer = std::make_shared<Buffer>(name + " vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, vertices.size(), sizeof(Vertex)), &vertices[0]);
		std::shared_ptr<Buffer> indexBuffer = std::make_shared<Buffer>(name + " index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, indices.size(), sizeof(uint32_t)), &indices[0]);

		for (auto& meshPrimitive : mesh.Primitives)
		{
			meshPrimitive.VertexBuffer = vertexBuffer;
			meshPrimitive.IndexBuffer = indexBuffer;
		}

		meshes.push_back(std::make_shared<Mesh>(mesh));
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

void ResourceManager::CalculateVertexTangents(Vertex& vert0, Vertex& vert1, Vertex& vert2)
{
	if (glm::abs(vert0.Normal).x > glm::abs(vert0.Normal.y))
		vert0.Tangent = glm::vec3(vert0.Normal.z, 0.0f, -vert0.Normal.x) / glm::sqrt(vert0.Normal.x * vert0.Normal.x + vert0.Normal.z * vert0.Normal.z);
	else
		vert0.Tangent = glm::vec3(0.0f, -vert0.Normal.z, vert0.Normal.y) / glm::sqrt(vert0.Normal.y * vert0.Normal.y + vert0.Normal.z * vert0.Normal.z);

	vert0.Bitangent = glm::cross(vert0.Normal, vert0.Tangent);

	if (glm::abs(vert1.Normal).x > glm::abs(vert1.Normal.y))
		vert1.Tangent = glm::vec3(vert1.Normal.z, 0.0f, -vert1.Normal.x) / glm::sqrt(vert1.Normal.x * vert1.Normal.x + vert1.Normal.z * vert1.Normal.z);
	else
		vert1.Tangent = glm::vec3(0.0f, -vert1.Normal.z, vert1.Normal.y) / glm::sqrt(vert1.Normal.y * vert1.Normal.y + vert1.Normal.z * vert1.Normal.z);

	vert1.Bitangent = glm::cross(vert1.Normal, vert1.Tangent);

	if (glm::abs(vert2.Normal).x > glm::abs(vert2.Normal.y))
		vert2.Tangent = glm::vec3(vert2.Normal.z, 0.0f, -vert2.Normal.x) / glm::sqrt(vert2.Normal.x * vert2.Normal.x + vert2.Normal.z * vert2.Normal.z);
	else
		vert2.Tangent = glm::vec3(0.0f, -vert2.Normal.z, vert2.Normal.y) / glm::sqrt(vert2.Normal.y * vert2.Normal.y + vert2.Normal.z * vert2.Normal.z);

	vert2.Bitangent = glm::cross(vert2.Normal, vert2.Tangent);
}
