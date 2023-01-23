#include "Pch.h"
#include "Scene/SceneObject.h"
#include "Components/TransformComponent.h"

#include <imgui/imgui.h>

SceneObject::SceneObject(std::size_t id, const std::string& name)
	: m_ID(id), m_Name(name)
{
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

void SceneObject::Render()
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (m_ComponentBitFlag & (1 << i))
		{
			m_Components[i]->Render();
		}
	}
}

void SceneObject::OnImGuiRender()
{
	// Push ID for this current scene object name
	ImGui::PushID(m_Name.c_str());

	if (ImGui::CollapsingHeader(m_Name.c_str()))
	{
		for (uint32_t i = 0; i < 8; ++i)
		{
			if (m_ComponentBitFlag & (1 << i))
			{
				ImGui::PushID(i);
				// Push ID for the current component ID
				ImGui::Indent(20.0f);
				m_Components[i]->OnImGuiRender();
				ImGui::Unindent(20.0f);
				ImGui::PopID();
			}
		}
	}

	ImGui::PopID();
}
