#include "Pch.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "Components/DirLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Resource/ResourceManager.h"
#include "Graphics/Mesh.h"
#include "Resource/Model.h"
#include "Application.h"

#include <imgui/imgui.h>

static std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Renderer::GetSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 250.0f, 0.0f), 60.0f, static_cast<float>(renderSettings.RenderResolution.x), static_cast<float>(renderSettings.RenderResolution.y), 0.1f, 10000.0f);
	m_AmbientLight = glm::vec3(0.0f);

	// Directional light
	DirectionalLightData dirLightData(glm::normalize(glm::vec3(0.0f, -100.0f, 0.0f)), glm::vec3(0.005f), glm::vec3(0.4f, 0.38f, 0.28f));
	std::size_t dirLight1 = AddSceneObject("DirectionalLight1");
	GetSceneObject(dirLight1).AddComponent<DirLightComponent>(dirLightData);

	// Pointlights
	PointLightData pointLightData(glm::vec3(1.0f, 0.007f, 0.0002f), glm::vec3(0.0001f, 0.00005f, 0.00005f), glm::vec3(10.0f, 0.0f, 0.0f));
	std::size_t pointLight1 = AddSceneObject("PointLight1");
	GetSceneObject(pointLight1).AddComponent<TransformComponent>(glm::vec3(-500.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight1).AddComponent<PointLightComponent>(pointLightData);

	pointLightData.Ambient = glm::vec3(0.00005f, 0.0001f, 0.00005f);
	pointLightData.Diffuse = glm::vec3(0.0f, 10.0f, 0.0f);
	std::size_t pointLight2 = AddSceneObject("PointLight2");
	GetSceneObject(pointLight2).AddComponent<TransformComponent>(glm::vec3(0.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight2).AddComponent<PointLightComponent>(pointLightData);

	pointLightData.Ambient = glm::vec3(0.00005f, 0.00005f, 0.0001f);
	pointLightData.Diffuse = glm::vec3(0.0f, 0.0f, 10.0f);
	std::size_t pointLight3 = AddSceneObject("PointLight3");
	GetSceneObject(pointLight3).AddComponent<TransformComponent>(glm::vec3(500.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight3).AddComponent<PointLightComponent>(pointLightData);

	// Spotlights
	SpotLightData spotLightData(glm::vec3(1.0f, 0.00014f, 0.00004f), 12.5f, 25.0f, glm::vec3(0.1f, 0.1f, 0.1f), glm::vec3(20.0f, 19.0f, 14.0f));
	std::size_t spotLight1 = AddSceneObject("SpotLight1");
	GetSceneObject(spotLight1).AddComponent<TransformComponent>(glm::vec3(-50.0f, 500.0f, 0.0f), glm::vec3(-45.0f, 0.0f, 0.0f));
	GetSceneObject(spotLight1).AddComponent<SpotLightComponent>(spotLightData);

	std::size_t spotLight2 = AddSceneObject("SpotLight2");
	GetSceneObject(spotLight2).AddComponent<TransformComponent>(glm::vec3(700.0f, 500.0f, 0.0f), glm::vec3(-45.0f, 0.0f, 0.0f));
	GetSceneObject(spotLight2).AddComponent<SpotLightComponent>(spotLightData);

	std::size_t spotLight3 = AddSceneObject("SpotLight3");
	GetSceneObject(spotLight3).AddComponent<TransformComponent>(glm::vec3(-800.0f, 500.0f, 0.0f), glm::vec3(-45.0f, 0.0f, 0.0f));
	GetSceneObject(spotLight3).AddComponent<SpotLightComponent>(spotLightData);

	std::size_t spotLight4 = AddSceneObject("SpotLight4");
	GetSceneObject(spotLight4).AddComponent<TransformComponent>(glm::vec3(-50.0f, 500.0f, 0.0f), glm::vec3(45.0f, 180.0f, 0.0f));
	GetSceneObject(spotLight4).AddComponent<SpotLightComponent>(spotLightData);

	std::size_t spotLight5 = AddSceneObject("SpotLight5");
	GetSceneObject(spotLight5).AddComponent<TransformComponent>(glm::vec3(700.0f, 500.0f, 0.0f), glm::vec3(45.0f, 180.0f, 0.0f));
	GetSceneObject(spotLight5).AddComponent<SpotLightComponent>(spotLightData);

	std::size_t spotLight6 = AddSceneObject("SpotLight6");
	GetSceneObject(spotLight6).AddComponent<TransformComponent>(glm::vec3(-800.0f, 500.0f, 0.0f), glm::vec3(45.0f, 180.0f, 0.0f));
	GetSceneObject(spotLight6).AddComponent<SpotLightComponent>(spotLightData);

	// Mesh objects
	auto& meshes1 = Application::Get().GetResourceManager()->GetModel("SponzaOld")->GetMeshes();
	for (auto& mesh : meshes1)
	{
		std::size_t sponzaMesh = AddSceneObject(mesh->GetName());
		GetSceneObject(sponzaMesh).AddComponent<TransformComponent>();
		GetSceneObject(sponzaMesh).AddComponent<MeshComponent>(mesh);
	}
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

std::size_t Scene::AddSceneObject(const std::string& name)
{
	auto& object = m_SceneObjects.emplace_back(std::make_unique<SceneObject>(m_SceneObjects.size(), name));
	return object->GetID();
}

SceneObject& Scene::GetSceneObject(std::size_t objectID)
{
	return *m_SceneObjects.at(objectID);
}
