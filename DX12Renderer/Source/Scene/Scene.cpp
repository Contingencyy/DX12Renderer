#include "Pch.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Components/MeshComponent.h"
#include "Components/DirLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Resource/ResourceManager.h"
#include "Resource/Model.h"
#include "Application.h"

#include <imgui/imgui.h>

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Renderer::GetSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f), 60.0f, static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y), 0.1f, 10000.0f);
	m_AmbientLight = glm::vec3(0.0f);

	// Directional light
	DirectionalLightData dirLightData(glm::normalize(glm::vec3(0.0f, -100.0f, 0.0f)), glm::vec3(0.005f), glm::vec3(0.4f, 0.38f, 0.28f));
	auto& dirLight1 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("DirectionalLight01"));
	dirLight1->AddComponent<DirLightComponent>(dirLightData);

	// Pointlights
	PointLightData pointLightData(glm::vec3(1.0f, 0.007f, 0.0002f), glm::vec3(0.0001f, 0.00005f, 0.00005f), glm::vec3(10.0f, 0.0f, 0.0f));
	auto& pointLight1 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("PointLight1", glm::vec3(-500.0f, 50.0f, 0.0f)));
	pointLight1->AddComponent<PointLightComponent>(pointLightData, pointLight1->GetTransform().GetPosition());

	pointLightData.Ambient = glm::vec3(0.00005f, 0.0001f, 0.00005f);
	pointLightData.Diffuse = glm::vec3(0.0f, 10.0f, 0.0f);
	auto& pointLight2 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("PointLight2", glm::vec3(0.0f, 50.0f, 0.0f)));
	pointLight2->AddComponent<PointLightComponent>(pointLightData, pointLight2->GetTransform().GetPosition());

	pointLightData.Ambient = glm::vec3(0.00005f, 0.00005f, 0.0001f);
	pointLightData.Diffuse = glm::vec3(0.0f, 0.0f, 10.0f);
	auto& pointLight3 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("PointLight3", glm::vec3(500.0f, 50.0f, 0.0f)));
	pointLight3->AddComponent<PointLightComponent>(pointLightData, pointLight3->GetTransform().GetPosition());

	// Spotlights
	SpotLightData spotLightData(glm::normalize(glm::vec3(0.0f, -1.0f, 1.0f)), glm::vec3(1.0f, 0.00014f, 0.00004f), 12.5f, 25.0f, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(20.0f, 19.0f, 14.0f));
	auto& spotLight1 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight01", glm::vec3(-50.0f, 500.0f, 0.0f)));
	spotLight1->AddComponent<SpotLightComponent>(spotLightData, spotLight1->GetTransform().GetPosition());
	//spotLight1->AddComponent<MeshComponent>(Application::Get().GetResourceManager()->GetModel("DamagedHelmet")->GetMeshes());

	auto& spotLight2 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight02", glm::vec3(700.0f, 500.0f, 0.0f)));
	spotLight2->AddComponent<SpotLightComponent>(spotLightData, spotLight2->GetTransform().GetPosition());

	auto& spotLight3 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight03", glm::vec3(-800.0f, 500.0f, 0.0f)));
	spotLight3->AddComponent<SpotLightComponent>(spotLightData, spotLight3->GetTransform().GetPosition());

	spotLightData.Direction = glm::normalize(glm::vec3(0.0f, -1.0f, -1.0f));

	auto& spotLight4 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight04", glm::vec3(-50.0f, 500.0f, 0.0f)));
	spotLight4->AddComponent<SpotLightComponent>(spotLightData, spotLight4->GetTransform().GetPosition());

	auto& spotLight5 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight05", glm::vec3(700.0f, 500.0f, 0.0f)));
	spotLight5->AddComponent<SpotLightComponent>(spotLightData, spotLight5->GetTransform().GetPosition());

	auto& spotLight6 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SpotLight06", glm::vec3(-800.0f, 500.0f, 0.0f)));
	spotLight6->AddComponent<SpotLightComponent>(spotLightData, spotLight6->GetTransform().GetPosition());

	// Mesh objects
	auto& meshObject1 = m_SceneObjects.emplace_back(std::make_unique<SceneObject>("SponzaOld", glm::vec3(0.0f), glm::vec3(0.0f), glm::vec3(1.0f)));
	meshObject1->AddComponent<MeshComponent>(Application::Get().GetResourceManager()->GetModel("SponzaOld")->GetMeshes());
}

Scene::~Scene()
{
}

void Scene::Update(float deltaTime)
{
	m_ActiveCamera.Update(deltaTime);

	for (auto& sceneObject : m_SceneObjects)
	{
		sceneObject->Update(deltaTime);
	}
}

void Scene::Render()
{
	SCOPED_TIMER("Scene::Render");

	for (auto& sceneObject : m_SceneObjects)
	{
		sceneObject->Render();
	}
}

void Scene::OnImGuiRender()
{
	ImGui::Begin("Scene");

	if (ImGui::CollapsingHeader("Camera"))
		m_ActiveCamera.OnImGuiRender();

	for (auto& sceneObject : m_SceneObjects)
	{
		sceneObject->OnImGuiRender();
	}

	ImGui::End();
}
