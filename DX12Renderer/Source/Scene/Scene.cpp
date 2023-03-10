#include "Pch.h"
#include "Scene/Scene.h"
#include "Scene/SceneObject.h"
#include "Components/TransformComponent.h"
#include "Components/MeshComponent.h"
#include "Components/DirLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/DebugRenderer.h"
#include "Resource/ResourceManager.h"

#include <imgui/imgui.h>

static std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;

Scene::Scene()
{
	Resolution renderRes = Renderer::GetRenderResolution();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 250.0f, 0.0f), 60.0f, static_cast<float>(renderRes.x), static_cast<float>(renderRes.y), 0.1f, 10000.0f);

	// Directional light
	DirectionalLightData dirLightData(glm::normalize(glm::vec3(0.0f, -1.0f, 0.0f)), glm::vec3(0.12f, 0.11f, 0.08f), glm::vec3(6.0f, 5.5f, 4.0f));
	std::size_t dirLight1 = AddSceneObject("DirectionalLight1");
	GetSceneObject(dirLight1).AddComponent<DirLightComponent>(dirLightData); 

	// Pointlights
	PointLightData pointLightData(glm::vec3(1.0f, 0.007f, 0.0002f), glm::vec3(10.0f, 0.0f, 0.0f));
	std::size_t pointLight1 = AddSceneObject("PointLight1");
	GetSceneObject(pointLight1).AddComponent<TransformComponent>(glm::vec3(-500.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight1).AddComponent<PointLightComponent>(pointLightData);

	pointLightData.Color = glm::vec3(0.0f, 10.0f, 0.0f);
	//pointLightData.Color = glm::vec3(10.0f, 10.0f, 10.0f);
	std::size_t pointLight2 = AddSceneObject("PointLight2");
	GetSceneObject(pointLight2).AddComponent<TransformComponent>(glm::vec3(0.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight2).AddComponent<PointLightComponent>(pointLightData);

	pointLightData.Color = glm::vec3(0.0f, 0.0f, 10.0f);
	std::size_t pointLight3 = AddSceneObject("PointLight3");
	GetSceneObject(pointLight3).AddComponent<TransformComponent>(glm::vec3(500.0f, 100.0f, 0.0f));
	GetSceneObject(pointLight3).AddComponent<PointLightComponent>(pointLightData);

	// Spotlights
	SpotLightData spotLightData(glm::vec3(1.0f, 0.00014f, 0.00004f), 12.5f, 25.0f, glm::vec3(20.0f, 19.0f, 14.0f));
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
	SpawnModelObject("SponzaOld", glm::vec3(), glm::vec3(), glm::vec3(125.0f));
	SpawnModelObject("Chess", glm::vec3(0.0f, 0.0f, -35.0f), glm::vec3(), glm::vec3(500.0f));
	//SpawnModelObject("DamagedHelmet", glm::vec3(0.0f, 300.0f, 0.0f), glm::vec3(), glm::vec3(100.0f));
	SpawnModelObject("Duck", glm::vec3(0.0f, 250.0f, 0.0f), glm::vec3(), glm::vec3());
	//SpawnModelObject("Spheres", glm::vec3(0.0f, 0.0f, -35.0f), glm::vec3(), glm::vec3(50.0f));
}

void Scene::Update(float deltaTime)
{
	m_ActiveCamera.Update(deltaTime);

	for (auto& sceneObject : m_SceneObjects)
	{
		if (sceneObject->GetName() == "Node20")
		{
			sceneObject->GetComponent<TransformComponent>().GetTransform().Rotate(glm::vec3(0.0f, glm::radians(45.0f) * deltaTime, 0.0f));
		}

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

void SpawnNodeMeshes(const Model& model, const Model::Node& node, const glm::mat4& parentTransform)
{
	// Spawn mesh objects for each mesh handle in the current node
	for (std::size_t nodeMeshIndex = 0; nodeMeshIndex < node.MeshHandles.size(); ++nodeMeshIndex)
	{
		const RenderResourceHandle nodeMeshHandle = node.MeshHandles[nodeMeshIndex];
		std::size_t nodeObject = Scene::AddSceneObject(node.Name + std::to_string(nodeMeshIndex));

		Scene::GetSceneObject(nodeObject).AddComponent<TransformComponent>(parentTransform);
		Scene::GetSceneObject(nodeObject).AddComponent<MeshComponent>(nodeMeshHandle);
	}

	// Now recurse over all child nodes of the current node
	for (auto& childNodeIndex : node.Children)
	{
		const Model::Node& childNode = model.Nodes[childNodeIndex];
		SpawnNodeMeshes(model, childNode, parentTransform * childNode.Transform);
	}
}

void Scene::SpawnModelObject(const std::string& modelName, const glm::vec3& translation,
	const glm::vec3& rotation, const glm::vec3& scale)
{
	auto model = ResourceManager::GetModel(modelName);
	auto& modelRootNodes = ResourceManager::GetModel(modelName)->RootNodes;

	Transform transform;
	transform.Translate(translation);
	transform.Rotate(rotation);
	transform.Scale(scale);

	// Go through all of the root nodes and spawn them and their child nodes, recursively
	for (auto& rootNodeIndex : modelRootNodes)
	{
		const Model::Node& rootNode = model->Nodes[rootNodeIndex];
		SpawnNodeMeshes(*model, rootNode, transform.GetTransformMatrix() * rootNode.Transform);
	}
}
