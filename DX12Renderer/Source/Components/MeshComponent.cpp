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
}

void MeshComponent::Render()
{
	const Transform& objectTransform = Scene::GetSceneObject(m_ObjectID).GetComponent<TransformComponent>(0).GetTransform();
	Renderer::Submit(m_Mesh, objectTransform.GetTransformMatrix());

	BoundingBox boundingBox = m_Mesh->GetBoundingBox();
	boundingBox.Min = glm::vec4(boundingBox.Min, 1.0f) * objectTransform.GetTransformMatrix();
	boundingBox.Max = glm::vec4(boundingBox.Max, 1.0f) * objectTransform.GetTransformMatrix();

	// Draw bounding box
	DebugRenderer::Submit(boundingBox.Min, glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), boundingBox.Min, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(boundingBox.Min, glm::vec3(boundingBox.Min.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Min.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Min.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Min.z), glm::vec3(boundingBox.Max.x, boundingBox.Max.y, boundingBox.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
}

void MeshComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader(m_Mesh->GetName().c_str()))
	{
		ImGui::Text("Mesh component");
	}
}
