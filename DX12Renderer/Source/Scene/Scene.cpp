#include "Pch.h"
#include "Scene/Scene.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Scene/SceneObject.h"
#include "Scene/MeshObject.h"
#include "Scene/LightObject.h"
#include "Resource/ResourceManager.h"
#include "Resource/Model.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Application::Get().GetRenderer()->GetRenderSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f), 60.0f, static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y));
	
	/*glm::vec3 position = glm::vec3(0.0f);
	glm::vec3 rotation = glm::vec3(90.0f, 0.0f, 180.0f);

	for (float y = -9.5f; y <= 9.5f; y += 1.0f)
	{
		for (float x = -9.5f; x <= 9.5f; x += 1.0f)
		{
			position.x = x * 2.0f;
			position.y = y * 2.0f;

			m_SceneObjects.push_back(std::make_unique<MeshObject>(Application::Get().GetResourceManager()->GetModel("DamagedHelmet")->GetMeshes(),
				"DamagedHelmet", position, rotation));
		}
	}*/

	// Pointlights
	PointlightData pointlightData(5000.0f, glm::vec3(0.0f, 0.001f, 0.0005f), glm::vec4(0.5f, 0.0f, 0.0f, 1.0f), glm::vec4(0.1f, 0.05f, 0.05f, 1.0f));
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(-25.0f, 25.0f, 0.0f)));

	pointlightData.Diffuse = glm::vec4(0.0f, 0.5f, 0.0f, 1.0f);
	pointlightData.Ambient = glm::vec4(0.05f, 0.1f, 0.05f, 1.0f);
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(0.0f, 25.0f, 0.0f)));

	pointlightData.Diffuse = glm::vec4(0.0f, 0.0f, 0.5f, 1.0f);
	pointlightData.Ambient = glm::vec4(0.05f, 0.05f, 0.1f, 1.0f);
	m_SceneObjects.push_back(std::make_unique<PointlightObject>(pointlightData, "Pointlight", glm::vec3(25.0f, 25.0f, 0.0f)));

	m_SceneObjects.push_back(std::make_unique<MeshObject>(Application::Get().GetResourceManager()->GetModel("SponzaOld")->GetMeshes(),
		"SponzaOld", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
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
		sceneObject->Render(m_ActiveCamera);
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
