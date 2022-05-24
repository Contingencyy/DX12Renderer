#pragma once
#include "Graphics/ParticleSystem.h"

class Scene
{
public:
	Scene();
	~Scene();

	void Update(float deltaTime);
	void Render();
	void GUIRender();

private:
	ParticleSystem m_ParticleSystem;
	ParticleProps m_ParticleProps;

	float m_TimeUntilParticleEmit = 100.0f;
	float m_ParticleEmitAccumulator = 0.0f;

};
