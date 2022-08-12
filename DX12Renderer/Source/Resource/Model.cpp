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
	for (uint32_t meshID = 0; meshID < glTFModel.meshes.size(); ++meshID)
	{
		auto& mesh = glTFModel.meshes[meshID];

		for (uint32_t primID = 0; primID < mesh.primitives.size(); ++primID)
		{
			auto& primitive = mesh.primitives[primID];

			// TODO: Hash should be the same if meshes are the same for instanced drawing
			std::string meshName = mesh.name.empty() ? "Unnamed" + std::to_string(meshID) + std::to_string(primID) : mesh.name;
			std::size_t meshHash = std::hash<std::string>{}(mesh.name + std::to_string(meshID) + std::to_string(primID));

			m_Meshes.push_back(std::make_shared<Mesh>(glTFModel, primitive, meshName, meshHash));
		}
	}
}
