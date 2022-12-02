#include "Pch.h"
#include "Scene/SceneObject.h"
#include "Resource/Model.h"
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
		if (m_Components[i].size() > 0)
		{
			for (auto& component : m_Components[i])
			{
				component->Update(deltaTime);
			}
		}
	}
}

void SceneObject::Render()
{
	for (uint32_t i = 0; i < 8; ++i)
	{
		if (m_Components[i].size() > 0)
		{
			for (auto& component : m_Components[i])
			{
				component->Render();
			}
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
			if (m_Components[i].size() > 0)
			{
				// Push ID for the current component ID
				ImGui::PushID(i);
				ImGui::Indent(20.0f);

				uint32_t compIndex = 0;
				for (auto& component : m_Components[i])
				{
					// Push ID for the current component index in the component vector
					ImGui::PushID(compIndex);

					component->OnImGuiRender();

					ImGui::PopID();
					compIndex++;
				}

				ImGui::Unindent(20.0f);
				ImGui::PopID();
			}
		}
	}

	ImGui::PopID();
}
