#pragma once
#include "Graphics/ParticleSystem.h"
#include "Camera.h"
#include "Scene/Light.h"

class SceneObject;

class Scene
{
public:
	Scene();
	~Scene();

	void Update(float deltaTime);
	void Render();
	void ImGuiRender();

	Camera& GetActiveCamera() { return m_ActiveCamera; }

private:
	Camera m_ActiveCamera;
	std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;
	std::vector<Pointlight> m_Pointlights;

	ParticleSystem m_ParticleSystem;
	ParticleProps m_ParticleProps;

	float m_TimeUntilParticleEmit = 100.0f;
	float m_ParticleEmitAccumulator = 0.0f;

};
