#include "Pch.h"
#include "Scene/MeshObject.h"

#include "Application.h"
#include "Graphics/Renderer.h"

MeshObject::MeshObject(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_Meshes(meshes)
{
}

MeshObject::~MeshObject()
{
}

void MeshObject::Update(float deltaTime)
{
}

void MeshObject::Render()
{
	for (auto& mesh : m_Meshes)
	{
		Application::Get().GetRenderer()->Submit(mesh, m_Transform.GetTransformMatrix());
	}
}
