#include "Pch.h"
#include "Components/MeshComponent.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Components/TransformComponent.h"

#include <imgui/imgui.h>

MeshComponent::MeshComponent(const std::shared_ptr<Mesh>& mesh)
	: m_Mesh(mesh)
{
}

MeshComponent::~MeshComponent()
{
}

void MeshComponent::Update(float deltaTime)
{
	const Transform& objectTransform = Scene::GetSceneObject(m_ObjectID).GetComponent<TransformComponent>(0).GetTransform();
	m_BoundingBox.Min = glm::vec4(m_Mesh->GetBoundingBox().Min, 1.0f) * glm::transpose(objectTransform.GetTransformMatrix());
	m_BoundingBox.Max = glm::vec4(m_Mesh->GetBoundingBox().Max, 1.0f) * glm::transpose(objectTransform.GetTransformMatrix());
}

void MeshComponent::Render()
{
	const Transform& objectTransform = Scene::GetSceneObject(m_ObjectID).GetComponent<TransformComponent>(0).GetTransform();
	Renderer::Submit(m_Mesh, m_BoundingBox, objectTransform.GetTransformMatrix());

	// Draw bounding box
	DebugRenderer::Submit(m_BoundingBox.Min, glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Min.z), glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), m_BoundingBox.Min, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(m_BoundingBox.Min, glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), glm::vec3(m_BoundingBox.Min.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Min.z), glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Min.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Min.z), glm::vec3(m_BoundingBox.Max.x, m_BoundingBox.Max.y, m_BoundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
}

void MeshComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader(m_Mesh->GetName().c_str()))
	{
		ImGui::Text("Mesh component");
	}
}
