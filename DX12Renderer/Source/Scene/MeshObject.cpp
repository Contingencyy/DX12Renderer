#include "Pch.h"
#include "Scene/MeshObject.h"
#include "Graphics/Mesh.h"

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

void MeshObject::Render(const Camera& camera)
{
	if (camera.IsFrustumCullingEnabled())
	{
		for (auto& mesh : m_Meshes)
		{
			Mesh::BoundingBox boundingBox = mesh->GetBoundingBox();

			boundingBox.Min = boundingBox.Min * m_Transform.GetScale() + m_Transform.GetPosition();
			boundingBox.Max = boundingBox.Max * m_Transform.GetScale() + m_Transform.GetPosition();

			if (camera.IsBoxInViewFrustum(boundingBox.Min, boundingBox.Max))
			{
				Application::Get().GetRenderer()->Submit(mesh, m_Transform.GetTransformMatrix());

				// Draw bounding box
				Application::Get().GetRenderer()->Submit(boundingBox.Min, glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), boundingBox.Min, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

				Application::Get().GetRenderer()->Submit(boundingBox.Min, glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
				Application::Get().GetRenderer()->Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
			}
		}
	}
	else
	{
		for (auto& mesh : m_Meshes)
		{
			Application::Get().GetRenderer()->Submit(mesh, m_Transform.GetTransformMatrix());
		}
	}
}
