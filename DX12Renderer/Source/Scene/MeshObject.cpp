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
			// TODO: View frustum culling should be checked against each mesh via bounding spheres
			Mesh::BoundingSphere boundingSphere = mesh->GetBoundingSphere();

			const glm::vec3& worldPosition = boundingSphere.Position * m_Transform.GetScale()/* * m_Transform.Rotation()*/;
			const glm::vec3& scaledRadius = boundingSphere.Radius * m_Transform.GetScale()/* * m_Transform.Rotation()*/;
			
			/*
			const glm::vec3& position = m_Transform.GetPosition();
			const glm::vec3& scale = m_Transform.GetScale();

			float radius = std::max(std::max(scale.x, scale.y), scale.z);
			*/
			
			float scaledBoundingRadius = std::max(std::max(scaledRadius.x, scaledRadius.y), scaledRadius.z);

			//if (camera.IsSphereInViewFrustum(position, radius))
			if (camera.IsSphereInViewFrustum(worldPosition, scaledBoundingRadius))
			{
				Application::Get().GetRenderer()->Submit(mesh, m_Transform.GetTransformMatrix());
				Application::Get().GetRenderer()->Submit(m_Transform.GetPosition(), m_Transform.GetPosition() + glm::vec3(0.0f, 1000.0f, 0.0f), glm::vec4(1.0f));
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
