#include "Pch.h"
#include "Scene/SceneObject.h"
#include "Resource/Model.h"

#include <imgui/imgui.h>

SceneObject::SceneObject(const std::string& name, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale, bool frustumCullable)
	: m_Name(name), m_FrustumCullable(frustumCullable)
{
	m_Transform.Translate(translation);
	m_Transform.Rotate(rotation);
	m_Transform.Scale(scale);
}

SceneObject::~SceneObject()
{
}

void SceneObject::Update(float deltaTime)
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (m_ComponentBitFlag & (1 << i))
		{
			m_Components[i]->Update(deltaTime);
		}
	}
}

void SceneObject::Render(const Camera& camera)
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (m_ComponentBitFlag & (1 << i))
		{
			m_Components[i]->Render(camera, m_Transform);
		}
	}
}

void SceneObject::OnImGuiRender()
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (m_ComponentBitFlag & (1 << i))
		{
			ImGui::PushID(m_Name.c_str());
			if (ImGui::CollapsingHeader(m_Name.c_str()))
			{
				ImGui::Indent(15.0f);
				m_Components[i]->OnImGuiRender();
				ImGui::Unindent(15.0f);
			}
			ImGui::PopID();
		}
	}
}
