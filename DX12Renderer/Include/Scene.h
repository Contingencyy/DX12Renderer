#pragma once
#include "Graphics/ParticleSystem.h"
#include "Camera.h"

class Model;

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
	std::vector<std::unique_ptr<Model>> m_Models;

	ParticleSystem m_ParticleSystem;
	ParticleProps m_ParticleProps;

	float m_TimeUntilParticleEmit = 100.0f;
	float m_ParticleEmitAccumulator = 0.0f;

};
