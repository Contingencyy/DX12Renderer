#include "Pch.h"
#include "Scene/Scene.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Scene/SceneObject.h"
#include "Scene/ModelObject.h"
#include "Scene/LightObject.h"
#include "Resource/ResourceManager.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Application::Get().GetRenderer()->GetRenderSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f), 60.0f, static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y));

	
	glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(90.0f, 0.0f, 180.0f);

	for (float y = -9.5f; y <= 9.5f; y += 1.0f)
	{
		for (float x = -9.5f; x <= 9.5f; x += 1.0f)
		{
			position.x = x * 2.0f;
			position.y = y * 2.0f;

			m_SceneObjects.push_back(std::make_unique<ModelObject>(Application::Get().GetResourceManager()->GetModel("DamagedHelmet"),
				"DamagedHelmet", position, rotation, glm::vec3(1.0f)));
		}
	}

	// Pointlights
	PointlightData pointlightData(500.0f, glm::vec3(0.0f, 0.1f, 0.01f), glm::vec4(0.5f, 0.0f, 0.0f, 1.0f), glm::vec4(0.1f, 0.05f, 0.05f, 1.0f));
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(-2.5f, 0.0f, -2.5f), glm::vec3(0.0f), glm::vec3(1.0f)));

	pointlightData.Diffuse = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
	pointlightData.Ambient = glm::vec4(0.05f, 0.1f, 0.05f, 1.0f);
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(0.0f, 0.0f, -2.5f), glm::vec3(0.0f), glm::vec3(1.0f)));

	pointlightData.Diffuse = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
	pointlightData.Ambient = glm::vec4(0.05f, 0.05f, 0.1f, 1.0f);
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(2.5f, 0.0f, -2.5f), glm::vec3(0.0f), glm::vec3(1.0f)));
}

Scene::~Scene()
{
}

void Scene::Update(float deltaTime)
{
	m_ActiveCamera.Update(deltaTime);

	for (auto& sceneObject : m_SceneObjects)
		sceneObject->Update(deltaTime);
}

void Scene::Render()
{
	SCOPED_TIMER("Scene::Render");

	for (auto& sceneObject : m_SceneObjects)
	{
		const glm::vec3& position = sceneObject->GetTransform().GetPosition();
		const glm::vec3& scale = sceneObject->GetTransform().GetScale();

		float radius = std::max(std::max(scale.x, scale.y), scale.z);
		if (m_ActiveCamera.IsSphereInViewFrustum(position, radius))
			sceneObject->Render();
	}
}

void Scene::ImGuiRender()
{
	std::string objectName = "";
	uint32_t idx = 0;

	ImGui::SetNextWindowSizeConstraints(ImVec2(75, 75), ImVec2(200, 400));
	ImGui::Begin("Scene");

	for (uint32_t i = 0; i < m_SceneObjects.size(); ++i)
	{
		auto& sceneObject = m_SceneObjects[i];
		ImGui::Text(sceneObject->GetName().c_str());
	}

	ImGui::End();
}
