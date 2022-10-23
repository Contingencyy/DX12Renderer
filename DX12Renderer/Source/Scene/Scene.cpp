#include "Pch.h"
#include "Scene/Scene.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Scene/SceneObject.h"
#include "Scene/MeshObject.h"
#include "Scene/LightObject.h"
#include "Application.h"
#include "Resource/ResourceManager.h"
#include "Resource/Model.h"

#include <imgui/imgui.h>

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Renderer::GetSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f), 60.0f, static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y));
	m_AmbientLight = glm::vec3(0.0f);

	// Directional light
	DirectionalLightData dirLightData(glm::normalize(glm::vec3(0.0f, -100.0f, 0.0f)), glm::vec3(0.005f), glm::vec3(0.4f, 0.38f, 0.28f));
	m_SceneObjects.push_back(std::make_unique<DirectionalLightObject>(dirLightData, "DirectionalLight"));

	// Pointlights
	//PointLightData pointlightData(glm::vec3(1.0f, 0.007f, 0.0002f), glm::vec3(0.0001f, 0.00005f, 0.00005f), glm::vec3(10.0f, 0.0f, 0.0f));
	//m_SceneObjects.push_back(std::make_unique<PointLightObject>(pointlightData, "PointLight", glm::vec3(-150.0f, 5.0f, 0.0f)));

	//pointlightData.Ambient = glm::vec3(0.00005f, 0.0001f, 0.00005f);
	//pointlightData.Diffuse = glm::vec3(0.0f, 10.0f, 0.0f);
	//m_SceneObjects.push_back(std::make_unique<PointLightObject>(pointlightData, "PointLight", glm::vec3(0.0f, 5.0f, 0.0f)));

	//pointlightData.Ambient = glm::vec3(0.00005f, 0.00005f, 0.0001f);
	//pointlightData.Diffuse = glm::vec3(0.0f, 0.0f, 10.0f);
	//m_SceneObjects.push_back(std::make_unique<PointLightObject>(pointlightData, "PointLight", glm::vec3(150.0f, 5.0f, 0.0f)));

	// Spotlights
	SpotLightData spotLightData(glm::normalize(glm::vec3(0.0f, -1.0f, 1.0f)), glm::vec3(1.0f, 0.00014f, 0.00004f), 12.5f, 25.0f, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(20.0f, 19.0f, 14.0f));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(-50.0f, 500.0f, 0.0f)));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(700.0f, 500.0f, 0.0f)));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(-800.0f, 500.0f, 0.0f)));
	spotLightData.Direction = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(-50.0f, 500.0f, 0.0f)));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(700.0f, 500.0f, 0.0f)));
	m_SceneObjects.push_back(std::make_unique<SpotLightObject>(spotLightData, "SpotLight", glm::vec3(-800.0f, 500.0f, 0.0f)));

	m_SceneObjects.push_back(std::make_unique<MeshObject>(Application::Get().GetResourceManager()->GetModel("SponzaOld")->GetMeshes(),
		"SponzaOld", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));

	/*for (float z = -5.0f; z <= 5.0f; z += 1.0f)
	{
		for (float x = -5.0f; x <= 5.0f; x += 1.0f)
		{
			m_SceneObjects.push_back(std::make_unique<MeshObject>(Application::Get().GetResourceManager()->GetModel("SponzaOld")->GetMeshes(),
				"SponzaOld", glm::vec3(x * 4000.0f, 0.0f, z * 2500.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
		}
	}*/
	
	/*glm::vec3 position = glm::vec3(0.0f);
	for (float y = -9.5f; y <= 9.5f; y += 1.0f)
	{
		for (float x = -9.5f; x <= 9.5f; x += 1.0f)
		{
			position.x = x * 20.0f;
			position.y = y * 20.0f;

			m_SceneObjects.push_back(std::make_unique<MeshObject>(Application::Get().GetResourceManager()->GetModel("DamagedHelmet")->GetMeshes(),
				"DamagedHelmet", position, glm::vec3(glm::radians(-90.0f), 0.0f, glm::radians(180.0f)), glm::vec3(10.0f)));
		}
	}*/
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

void Scene::OnImGuiRender()
{
	ImGui::Begin("Scene");
	m_ActiveCamera.OnImGuiRender();
	ImGui::End();

	/*std::string objectName = "";
	uint32_t idx = 0;

	ImGui::SetNextWindowSizeConstraints(ImVec2(75, 75), ImVec2(200, 400));
	ImGui::Begin("Scene");

	for (uint32_t i = 0; i < m_SceneObjects.size(); ++i)
	{
		auto& sceneObject = m_SceneObjects[i];
		ImGui::Text(sceneObject->GetName().c_str());
	}

	ImGui::End();*/
}
