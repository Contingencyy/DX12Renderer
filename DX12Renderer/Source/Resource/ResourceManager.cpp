#include "Pch.h"
#include "Resource/ResourceManager.h"
#include "Resource/FileLoader.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/Texture.h"

#include "mikkt/mikktspace.h"

struct Vertex
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
	glm::vec3 Normal;
	glm::vec3 Tangent;
	glm::vec3 Bitangent;
};

struct LoadedMesh
{
	std::vector<Vertex>* Vertices;
	std::vector<uint32_t>* Indices;
};

class TangentCalculator
{
public:
	TangentCalculator()
	{
		m_MikkTInterface.m_getNumFaces = GetNumFaces;
		m_MikkTInterface.m_getNumVerticesOfFace = GetNumVerticesOfFace;

		m_MikkTInterface.m_getNormal = GetNormal;
		m_MikkTInterface.m_getPosition = GetPosition;
		m_MikkTInterface.m_getTexCoord = GetTexCoord;
		m_MikkTInterface.m_setTSpaceBasic = SetTSpaceBasic;

		m_MikkTContext.m_pInterface = &m_MikkTInterface;
	}

	void Calculate(LoadedMesh* loadedMesh)
	{
		m_MikkTContext.m_pUserData = loadedMesh;
		genTangSpaceDefault(&m_MikkTContext);
	}

private:
	static int GetNumFaces(const SMikkTSpaceContext* context)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);
		return (uint32_t)(*loadedMesh->Indices).size() / 3;
	}

	static int GetVertexIndex(const SMikkTSpaceContext* context, int iFace, int iVert)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);

		uint32_t faceSize = GetNumVerticesOfFace(context, iFace);
		uint32_t indicesIndex = (iFace * faceSize) + iVert;

		return (*loadedMesh->Indices)[indicesIndex];
	}

	static int GetNumVerticesOfFace(const SMikkTSpaceContext* context, int iFace)
	{
		// We only expect triangles (for now), so always return 3
		return 3;
	}

	static void GetPosition(const SMikkTSpaceContext* context, float outpos[], int iFace, int iVert)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);

		uint32_t index = GetVertexIndex(context, iFace, iVert);
		const Vertex& vertex = (*loadedMesh->Vertices)[index];

		outpos[0] = vertex.Position.x;
		outpos[1] = vertex.Position.y;
		outpos[2] = vertex.Position.z;
	}

	static void GetNormal(const SMikkTSpaceContext* context, float outnormal[], int iFace, int iVert)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);

		uint32_t index = GetVertexIndex(context, iFace, iVert);
		const Vertex& vertex = (*loadedMesh->Vertices)[index];

		outnormal[0] = vertex.Normal.x;
		outnormal[1] = vertex.Normal.y;
		outnormal[2] = vertex.Normal.z;
	}

	static void GetTexCoord(const SMikkTSpaceContext* context, float outuv[], int iFace, int iVert)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);

		uint32_t index = GetVertexIndex(context, iFace, iVert);
		const Vertex& vertex = (*loadedMesh->Vertices)[index];

		outuv[0] = vertex.TexCoord.x;
		outuv[1] = vertex.TexCoord.y;
	}

	static void SetTSpaceBasic(const SMikkTSpaceContext* context, const float tangentu[], float fSign, int iFace, int iVert)
	{
		LoadedMesh* loadedMesh = static_cast<LoadedMesh*>(context->m_pUserData);

		uint32_t index = GetVertexIndex(context, iFace, iVert);
		Vertex& vertex = (*loadedMesh->Vertices)[index];

		vertex.Tangent.x = tangentu[0];
		vertex.Tangent.y = tangentu[1];
		vertex.Tangent.z = tangentu[2];

		vertex.Bitangent = glm::cross(vertex.Normal, vertex.Tangent) * fSign;
	}

private:
	SMikkTSpaceInterface m_MikkTInterface = {};
	SMikkTSpaceContext m_MikkTContext = {};

};

std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;
TangentCalculator m_TangentCalculator;

glm::mat4 MakeNodeTransform(const tinygltf::Node& gltfnode)
{
	glm::mat4 transform = glm::identity<glm::mat4>();

	if (gltfnode.matrix.size() == 16)
	{
		transform[0] = glm::vec4(gltfnode.matrix[0], gltfnode.matrix[1], gltfnode.matrix[2], gltfnode.matrix[3]);
		transform[1] = glm::vec4(gltfnode.matrix[4], gltfnode.matrix[5], gltfnode.matrix[6], gltfnode.matrix[7]);
		transform[2] = glm::vec4(gltfnode.matrix[8], gltfnode.matrix[9], gltfnode.matrix[10], gltfnode.matrix[11]);
		transform[3] = glm::vec4(gltfnode.matrix[12], gltfnode.matrix[13], gltfnode.matrix[14], gltfnode.matrix[15]);
	}
	else
	{
		glm::vec3 translation(0.0f);
		glm::vec4 rotation(0.0f);
		glm::vec3 scale(1.0f);

		if (gltfnode.translation.size() == 3)
			translation = glm::vec3(gltfnode.translation[0], gltfnode.translation[1], gltfnode.translation[2]);
		if (gltfnode.rotation.size() == 4)
			rotation = glm::vec4(gltfnode.rotation[0], gltfnode.rotation[1], gltfnode.rotation[2], gltfnode.rotation[3]);
		if (gltfnode.scale.size() == 3)
			scale = glm::vec3(gltfnode.scale[0], gltfnode.scale[1], gltfnode.scale[2]);

		glm::mat4 translationMatrix = glm::translate(glm::identity<glm::mat4>(), translation);
		glm::mat4 rotationMatrix = glm::mat4_cast(glm::fquat(rotation));
		glm::mat4 scaleMatrix = glm::scale(glm::identity<glm::mat4>(), scale);

		transform = translationMatrix * rotationMatrix * scaleMatrix;
	}

	return transform;
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

	std::vector<RenderResourceHandle> meshHandles;
	std::vector<Vertex> vertices;
	std::vector<uint32_t> indices;

	for (auto& gltfMesh : tinygltf.meshes)
	{
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

			vertices.resize(vertexPosAccessor.count);

			// Construct a vertex from the primitive attributes data and add it to all vertices
			for (std::size_t i = 0; i < vertexPosAccessor.count; ++i)
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

				for (std::size_t i = 0; i < vertexTangentAccessor.count; ++i)
				{
					Vertex& vert0 = vertices[i];

					vert0.Tangent = glm::normalize(*pVertexTangentData);
					vert0.Bitangent = glm::normalize(glm::cross(vert0.Normal, vert0.Tangent) * pVertexTangentData->w);

					pVertexTangentData++;
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

			indices.resize(indexAccessor.count);

			// Get indices for current primitive/mesh and add it to all indices
			if (indicesByteSize == 2)
			{
				const uint16_t* pIndexData = reinterpret_cast<const uint16_t*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);

				for (std::size_t i = 0; i < indexAccessor.count; ++i)
				{
					indices[i] = static_cast<uint32_t>(*pIndexData++);
				}
			}
			else if (indicesByteSize == 4)
			{
				const uint32_t* pIndexData = reinterpret_cast<const uint32_t*>(&indexBuffer.data[0] + indexBufferView.byteOffset + indexAccessor.byteOffset);

				for (std::size_t i = 0; i < indexAccessor.count; ++i)
				{
					indices[i] = *pIndexData++;
				}
			}

			// Calculate missing tangents and bitangents
			if (vertexTangentAttrib == gltfPrim.attributes.end())
			{
				LoadedMesh loadedMesh = {};
				loadedMesh.Vertices = &vertices;
				loadedMesh.Indices = &indices;

				m_TangentCalculator.Calculate(&loadedMesh);
			}

			MeshDesc meshDesc = {};
			meshDesc.DebugName = gltfMesh.name + std::to_string(meshHandles.size());
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

	// Load all nodes and their transforms
	std::vector<Model::Node> nodes;
	if (tinygltf.nodes.size() > 0)
	{
		for (std::size_t nodeIndex = 0; nodeIndex < tinygltf.nodes.size(); ++nodeIndex)
		{
			const tinygltf::Node& gltfnode = tinygltf.nodes[nodeIndex];
			Model::Node node = {};

			if (gltfnode.mesh >= 0)
			{
				std::size_t meshIndex = gltfnode.mesh;
				std::size_t numMeshes = tinygltf.meshes[gltfnode.mesh].primitives.size();

				node.MeshHandles = std::vector<RenderResourceHandle>(meshHandles.begin() + meshIndex, meshHandles.begin() + meshIndex + numMeshes);
			}

			node.Transform = MakeNodeTransform(gltfnode);
			node.Name = gltfnode.name.empty() ? "Node" + std::to_string(nodeIndex) : gltfnode.name;
			node.Children = std::vector<std::size_t>(gltfnode.children.begin(), gltfnode.children.end());

			nodes.emplace_back(node);
		}
	}
	else
	{
		for (std::size_t meshIndex = 0; meshIndex < meshHandles.size(); ++meshIndex)
		{
			const RenderResourceHandle& meshHandle = meshHandles[meshIndex];

			Model::Node node = {};
			node.MeshHandles.emplace_back(meshHandle);
			node.Transform = glm::identity<glm::mat4>();
			node.Name = "Node" + std::to_string(meshIndex);

			nodes.emplace_back(node);
		}
	}

	std::vector<std::size_t> rootNodes;
	if (tinygltf.scenes.size() > 0)
	{
		// Only load the default scene's root nodes
		rootNodes = std::vector<std::size_t>(tinygltf.scenes[tinygltf.defaultScene].nodes.begin(), tinygltf.scenes[tinygltf.defaultScene].nodes.end());
	}
	else
	{
		for (std::size_t i = 0; i < nodes.size(); ++i)
			rootNodes.emplace_back(i);
	}

	Model model = {};
	model.Nodes = nodes;
	model.RootNodes = rootNodes;
	model.Name = name;
	m_Models.insert(std::pair<std::string, std::shared_ptr<Model>>(name, std::make_shared<Model>(model)));
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
