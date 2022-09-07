#include "Pch.h"
#include "Resource/Model.h"
#include "Graphics/Mesh.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

Model::Model(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& name)
	: m_Meshes(meshes), m_Name(name)
{
}

Model::~Model()
{
}
