#include "Pch.h"
#include "Components/MeshComponent.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/Mesh.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"

#include <imgui/imgui.h>

MeshComponent::MeshComponent(const std::vector<std::shared_ptr<Mesh>>& meshes)
	: m_Meshes(meshes)
{
}

MeshComponent::~MeshComponent()
{
}

void MeshComponent::Update(float deltaTime)
{
}

void MeshComponent::Render(const Transform& transform)
{
	for (auto& mesh : m_Meshes)
	{
		Renderer::Submit(mesh, transform.GetTransformMatrix());

		Mesh::BoundingBox boundingBox = mesh->GetBoundingBox();

		boundingBox.Min = boundingBox.Min * transform.GetScale() + transform.GetPosition();
		boundingBox.Max = boundingBox.Max * transform.GetScale() + transform.GetPosition();

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
}

void MeshComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Mesh"))
	{
		ImGui::Text("Mesh component");
	}
}
