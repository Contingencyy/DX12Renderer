#include "Pch.h"
#include "Components/MeshComponent.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Components/TransformComponent.h"

#include <imgui/imgui.h>

MeshComponent::MeshComponent(RenderResourceHandle meshHandle)
	: m_Mesh(meshHandle)
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
	const Transform& objectTransform = Scene::GetSceneObject(m_ObjectID).GetComponent<TransformComponent>().GetTransform();
	Renderer::Submit(m_Mesh, objectTransform.GetTransformMatrix());

	// Draw bounding box
	/*DebugRenderer::Submit(bb.Min, glm::vec3(bb.Max.x, bb.Min.y, bb.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Min.y, bb.Min.z), glm::vec3(bb.Max.x, bb.Max.y, bb.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Max.y, bb.Min.z), glm::vec3(bb.Min.x, bb.Max.y, bb.Min.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Min.x, bb.Max.y, bb.Min.z), bb.Min, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(glm::vec3(bb.Min.x, bb.Min.y, bb.Max.z), glm::vec3(bb.Max.x, bb.Min.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Min.y, bb.Max.z), glm::vec3(bb.Max.x, bb.Max.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Max.y, bb.Max.z), glm::vec3(bb.Min.x, bb.Max.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Min.x, bb.Max.y, bb.Max.z), glm::vec3(bb.Min.x, bb.Min.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	DebugRenderer::Submit(bb.Min, glm::vec3(bb.Min.x, bb.Min.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Min.x, bb.Max.y, bb.Min.z), glm::vec3(bb.Min.x, bb.Max.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Min.y, bb.Min.z), glm::vec3(bb.Max.x, bb.Min.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	DebugRenderer::Submit(glm::vec3(bb.Max.x, bb.Max.y, bb.Min.z), glm::vec3(bb.Max.x, bb.Max.y, bb.Max.z), glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));*/
}

void MeshComponent::OnImGuiRender()
{
	/*if (ImGui::CollapsingHeader(("Mesh Component | " + m_Mesh.Name).c_str()))
	{
		ImGui::Indent(20.0f);

		for (std::size_t primitiveIndex = 0; primitiveIndex < m_Mesh.Primitives.size(); ++primitiveIndex)
		{
			MeshPrimitive& primitive = m_Mesh->Primitives[primitiveIndex];
			ImGui::PushID(primitiveIndex);

			if (ImGui::CollapsingHeader(("Mesh Primitive | " + primitive.DebugName).c_str()))
			{
				ImGui::Indent(20.0f);

				if (ImGui::CollapsingHeader("Material"))
				{
					ImGui::Indent(20.0f);
					ImGui::DragFloat("Metalness", &primitive.Material.MetalnessFactor, 0.001f, 0.0f, 1.0f, "%.3f");
					ImGui::DragFloat("Roughness", &primitive.Material.RoughnessFactor, 0.001f, 0.0f, 1.0f, "%.3f");
					ImGui::Unindent(20.0f);
				}

				ImGui::Unindent(20.0f);
			}

			ImGui::PopID();
		}

		ImGui::Unindent(20.0f);
	}*/
}
