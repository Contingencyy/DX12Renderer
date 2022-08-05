#include "Pch.h"
#include "Resource/Model.h"
#include "Graphics/Mesh.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

Model::Model(const tinygltf::Model& glTFModel, const std::string& name)
	: m_Name(name)
{
	CreateMeshes(glTFModel);
}

Model::~Model()
{
}

void Model::CreateMeshes(const tinygltf::Model& glTFModel)
{
	for (uint32_t meshID = 0; meshID < glTFModel.meshes[0].primitives.size(); ++meshID)
	{
		m_Meshes.push_back(std::make_shared<Mesh>(glTFModel, meshID));
	}
}
