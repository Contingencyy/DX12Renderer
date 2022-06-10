#include "Pch.h"
#include "Scene.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "SceneObject.h"
#include "ResourceManager.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Scene::Scene()
{
	Renderer::RenderSettings renderSettings = Application::Get().GetRenderer()->GetRenderSettings();
	m_ActiveCamera = Camera(glm::vec3(0.0f, 0.0f, -5.0f), 60.0f, static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y));

	/*m_ParticleProps.ColorBegin = { 254 / 255.0f, 182 / 255.0f, 40 / 255.0f, 1.0f };
	m_ParticleProps.ColorEnd = { 254 / 255.0f, 59 / 255.0f, 10 / 255.0f, 1.0f };
	m_ParticleProps.BaseRotation = 0.0f, m_ParticleProps.BaseRotationVariation = 0.5f;
	m_ParticleProps.SizeBegin = 0.5f, m_ParticleProps.SizeVariation = 0.3f, m_ParticleProps.SizeEnd = 0.0f;
	m_ParticleProps.LifeTime = 5.0f;
	m_ParticleProps.Velocity = { 0.0f, 1.0f };
	m_ParticleProps.VelocityVariation = { 0.2f, 0.2f };
	m_ParticleProps.Position = { 0.0f, 0.0f };*/

	glm::mat4 transform = glm::identity<glm::mat4>();
	for (int y = -1; y <= 1; ++y)
	{
		for (int x = -1; x <= 1; ++x)
		{
			glm::mat4 currentTransform = glm::translate(transform, glm::vec3(static_cast<float>(x) * 2.0f, static_cast<float>(y) * 2.0f, 0.0f));
			currentTransform = glm::rotate(currentTransform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
			currentTransform = glm::rotate(currentTransform, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
			currentTransform = glm::rotate(currentTransform, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

			m_SceneObjects.push_back(std::make_unique<SceneObject>(Application::Get().GetResourceManager()->GetModel("DamagedHelmet"), currentTransform));
		}
	}
}

Scene::~Scene()
{
}

void Scene::Update(float deltaTime)
{
	m_ActiveCamera.Update(deltaTime);

	for (auto& sceneObject : m_SceneObjects)
		sceneObject->Update(deltaTime);

	/*m_ParticleEmitAccumulator += deltaTime * 1000.0f;
	while (m_ParticleEmitAccumulator >= m_TimeUntilParticleEmit)
	{
		m_ParticleSystem.Emit(m_ParticleProps);
		m_ParticleEmitAccumulator = fmodf(m_ParticleEmitAccumulator, m_TimeUntilParticleEmit);
	}

	m_ParticleSystem.Update(deltaTime);*/
}

void Scene::Render()
{
	for (auto& sceneObject : m_SceneObjects)
		sceneObject->Render();

	//m_ParticleSystem.Render();
}

void Scene::ImGuiRender()
{
	/*ImGui::Begin("Particle system");

	ImGui::Text("Active particles: %u", m_ParticleSystem.GetNumActiveParticles());

	ImGui::ColorEdit4("Color begin", &m_ParticleProps.ColorBegin.x);
	ImGui::ColorEdit4("Color end", &m_ParticleProps.ColorEnd.x);

	ImGui::DragFloat("Base rotation", &m_ParticleProps.BaseRotation, 0.01f, 0.0f);
	ImGui::DragFloat("Base rotation variation", &m_ParticleProps.BaseRotationVariation, 0.01f, 0.0f);

	ImGui::DragFloat("Size begin", &m_ParticleProps.SizeBegin, 0.01f, 0.0f);
	ImGui::DragFloat("Size variation", &m_ParticleProps.SizeVariation, 0.01f, 0.0f);
	ImGui::DragFloat("Size end", &m_ParticleProps.SizeEnd, 0.01f, 0.0f);

	ImGui::DragFloat2("Position begin", &m_ParticleProps.Position.x, 0.01f, 0.0f);
	ImGui::DragFloat2("Velocity", &m_ParticleProps.Velocity.x, 0.01f, 0.0f);
	ImGui::DragFloat2("Velocity variation", &m_ParticleProps.VelocityVariation.x, 0.01f, 0.0f);

	ImGui::DragFloat("Lifetime", &m_ParticleProps.LifeTime, 0.05f, 0.0f);

	ImGui::End();*/
}
