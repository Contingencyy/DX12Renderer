#include "Pch.h"
#include "Resource/ResourceManager.h"
#include "Resource/FileLoader.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/Texture.h"

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
	textureDesc.DebugName = name;
	m_Textures.insert(std::pair<std::string, std::shared_ptr<Texture>>(name, std::make_shared<Texture>(textureDesc)));

	delete imageInfo.Data;

	LOG_INFO("[ResourceManager] Loaded texture: " + filepath);
}

void ResourceManager::LoadModel(const std::string& filepath, const std::string& name)
{
	const tinygltf::Model& tinygltf = FileLoader::LoadGLTFModel(filepath);

	// Create all textures
	std::vector<RenderResourceHandle> materialHandles;
	materialHandles.reserve(tinygltf.materials.size());

	for (uint32_t matIndex = 0; matIndex < tinygltf.materials.size(); ++matIndex)
	{
		auto& gltfMaterial = tinygltf.materials[matIndex];

		int albedoTextureIndex = gltfMaterial.pbrMetallicRoughness.baseColorTexture.index;
		int normalTextureIndex = gltfMaterial.normalTexture.index;
		int metallicRoughnessTextureIndex = gltfMaterial.pbrMetallicRoughness.metallicRoughnessTexture.index;

		MaterialDesc materialDesc = {};

		if (albedoTextureIndex >= 0)
		{
			const tinygltf::Image& albedoImage = tinygltf.images[tinygltf.textures[albedoTextureIndex].source];

			materialDesc.AlbedoDesc.Usage = TextureUsage::TEXTURE_USAGE_READ;
			materialDesc.AlbedoDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_SRGB;
			materialDesc.AlbedoDesc.Width = (uint32_t)albedoImage.width;
			materialDesc.AlbedoDesc.Height = (uint32_t)albedoImage.height;
			materialDesc.AlbedoDesc.NumMips = CalculateTotalMipCount(materialDesc.AlbedoDesc.Width, materialDesc.AlbedoDesc.Height);
			materialDesc.AlbedoDesc.DataPtr = &albedoImage.image[0];
			materialDesc.AlbedoDesc.DebugName = name + " albedo texture";
		}

		if (normalTextureIndex >= 0)
		{
			const tinygltf::Image& normalImage = tinygltf.images[tinygltf.textures[normalTextureIndex].source];

			materialDesc.NormalDesc.Usage = TextureUsage::TEXTURE_USAGE_READ;
			materialDesc.NormalDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
			materialDesc.NormalDesc.Width = (uint32_t)normalImage.width;
			materialDesc.NormalDesc.Height = (uint32_t)normalImage.height;
			materialDesc.NormalDesc.NumMips = CalculateTotalMipCount(materialDesc.NormalDesc.Width, materialDesc.NormalDesc.Height);
			materialDesc.NormalDesc.DataPtr = &normalImage.image[0];
			materialDesc.NormalDesc.DebugName = name + " normal texture";
		}

		if (metallicRoughnessTextureIndex >= 0)
		{
			const tinygltf::Image& metallicRoughnessImage = tinygltf.images[tinygltf.textures[metallicRoughnessTextureIndex].source];

			materialDesc.MetallicRoughnessDesc.Usage = TextureUsage::TEXTURE_USAGE_READ;
			materialDesc.MetallicRoughnessDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
			materialDesc.MetallicRoughnessDesc.Width = (uint32_t)metallicRoughnessImage.width;
			materialDesc.MetallicRoughnessDesc.Height = (uint32_t)metallicRoughnessImage.height;
			materialDesc.MetallicRoughnessDesc.NumMips = CalculateTotalMipCount(materialDesc.MetallicRoughnessDesc.Width, materialDesc.MetallicRoughnessDesc.Height);
			materialDesc.MetallicRoughnessDesc.DataPtr = &metallicRoughnessImage.image[0];
			materialDesc.MetallicRoughnessDesc.DebugName = name + " metallic roughness texture";
		}

		materialDesc.Metalness = gltfMaterial.pbrMetallicRoughness.metallicFactor;
		materialDesc.Roughness = gltfMaterial.pbrMetallicRoughness.roughnessFactor;
		materialDesc.Transparency = gltfMaterial.alphaMode.compare("OPAQUE") == 0 ? TransparencyMode::OPAQUE : TransparencyMode::TRANSPARENT;

		materialHandles.emplace_back(Renderer::CreateMaterial(materialDesc));
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

	std::vector<RenderResourceHandle> meshHandles;

	std::size_t currentStartVertex = 0;
	std::size_t currentStartIndex = 0;

	for (auto& gltfMesh : tinygltf.meshes)
	{
		std::vector<RenderResourceHandle> meshPrimitiveHandles;
		meshPrimitiveHandles.reserve(gltfMesh.primitives.size());

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

			std::vector<Vertex> vertices(vertexPosAccessor.count);

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

			std::vector<uint32_t> indices(indexAccessor.count);

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

			MeshDesc meshDesc = {};
			meshDesc.DebugName = gltfMesh.name + std::to_string(meshPrimitiveHandles.size());
			meshDesc.VertexBufferDesc.Usage = BufferUsage::BUFFER_USAGE_VERTEX;
			meshDesc.VertexBufferDesc.NumElements = vertices.size();
			meshDesc.VertexBufferDesc.ElementSize = sizeof(Vertex);
			meshDesc.VertexBufferDesc.DataPtr = &vertices[0];
			meshDesc.VertexBufferDesc.DebugName = meshDesc.DebugName + " vertex buffer";
			meshDesc.IndexBufferDesc.Usage = BufferUsage::BUFFER_USAGE_INDEX;
			meshDesc.IndexBufferDesc.NumElements = indices.size();
			meshDesc.IndexBufferDesc.ElementSize = sizeof(uint32_t);
			meshDesc.IndexBufferDesc.DataPtr = &indices[0];
			meshDesc.IndexBufferDesc.DebugName = meshDesc.DebugName + " index buffer";
			meshDesc.MaterialHandle = materialHandles[gltfPrim.material];
			meshDesc.BB.Min = (glm::vec3)minBounds;
			meshDesc.BB.Max = (glm::vec3)maxBounds;

			meshHandles.emplace_back(Renderer::CreateMesh(meshDesc));
		}
	}

	m_Models.insert(std::pair<std::string, std::shared_ptr<Model>>(name, std::make_shared<Model>(meshHandles, name)));
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
