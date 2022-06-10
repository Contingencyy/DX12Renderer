#include "Pch.h"
#include "Graphics/Model.h"
#include "Graphics/Shader.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "ResourceLoader.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/PipelineState.h"

Model::Model(const tinygltf::Model& glTFModel)
{
	CreatePipelineState();

	CreateBuffers(glTFModel);
	CreateTextures(glTFModel);
}

Model::~Model()
{
}

void Model::CreatePipelineState()
{
	m_PipelineState = std::make_unique<PipelineState>(L"Resources/Shaders/Default_VS.hlsl", L"Resources/Shaders/Default_PS.hlsl");
}

void Model::CreateBuffers(const tinygltf::Model& glTFModel)
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0" };

	for (const std::string& attributeName : attributeNames)
	{
		auto iter = glTFModel.meshes[0].primitives[0].attributes.find(attributeName);
		if (iter != glTFModel.meshes[0].primitives[0].attributes.end())
		{
			uint32_t attributeIndex = iter->second;
			auto& accessor = glTFModel.accessors[attributeIndex];
			auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
			auto& buffer = glTFModel.buffers[bufferView.buffer];

			const unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
			m_Buffers.push_back(std::make_shared<Buffer>(BufferDesc(), accessor.count, accessor.ByteStride(bufferView), dataPtr));
		}
		else
		{
			ASSERT(false, "glTF model mesh does not contain attribute " + attributeName);
		}
	}

	// Load indices from glTF model
	uint32_t indiciesIndex = glTFModel.meshes[0].primitives[0].indices;
	auto& accessor = glTFModel.accessors[indiciesIndex];
	auto& bufferView = glTFModel.bufferViews[accessor.bufferView];
	auto& buffer = glTFModel.buffers[bufferView.buffer];

	const unsigned char* dataPtr = &glTFModel.buffers[0].data[0] + bufferView.byteOffset;

	m_Buffers.push_back(std::make_shared<Buffer>(BufferDesc(), accessor.count, accessor.ByteStride(bufferView), dataPtr));
}

void Model::CreateTextures(const tinygltf::Model& glTFModel)
{
	auto& material = glTFModel.materials[0];
	uint32_t albedoTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
	uint32_t imageIndex = glTFModel.textures[albedoTextureIndex].source;

	m_Textures.push_back(std::make_shared<Texture>(TextureDesc(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE,
		glTFModel.images[imageIndex].width, glTFModel.images[imageIndex].height), &glTFModel.images[imageIndex].image[0]));
}
