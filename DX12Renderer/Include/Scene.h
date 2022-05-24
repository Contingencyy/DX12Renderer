#pragma once
#include "Graphics/ParticleSystem.h"
#include "Camera.h"

class Scene
{
public:
	Scene();
	~Scene();

	void Update(float deltaTime);
	void Render();
	void GUIRender();

	Camera& GetActiveCamera() { return m_ActiveCamera; }

private:
	Camera m_ActiveCamera;

	ParticleSystem m_ParticleSystem;
	ParticleProps m_ParticleProps;

	float m_TimeUntilParticleEmit = 100.0f;
	float m_ParticleEmitAccumulator = 0.0f;

};
