#include "Pch.h"
#include "Components/TransformComponent.h"

#include <imgui/imgui.h>

TransformComponent::TransformComponent(const glm::mat4& matrix)
{
	m_Transform = Transform(matrix);
}

TransformComponent::TransformComponent(const glm::vec3& translation,
	const glm::vec3& rotation, const glm::vec3& scale)
{
	m_Transform.SetTranslation(translation);
	m_Transform.SetRotation(glm::radians(rotation));
	m_Transform.SetScale(scale);
}

TransformComponent::~TransformComponent()
{
}

void TransformComponent::Update(float deltaTime)
{
}

void TransformComponent::Render()
{
}

void TransformComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Transform Component"))
	{
		ImGui::Indent(20.0f);

		glm::vec3 translation = m_Transform.GetPosition();
		if (ImGui::DragFloat3("Translation", &translation.x, 0.1f))
			m_Transform.SetTranslation(translation);

		glm::vec3 rotation = glm::degrees(m_Transform.GetRotation());
		if (ImGui::DragFloat3("Rotation", &rotation.x, 0.01f, -360.0f, 360.0f))
			m_Transform.SetRotation(glm::radians(rotation));

		glm::vec3 scale = m_Transform.GetScale();
		if (ImGui::DragFloat3("Scale", &scale.x, 0.01f, 0.001f, 1000000.0f))
			m_Transform.SetScale(scale);

		ImGui::Unindent(20.0f);
	}
}
