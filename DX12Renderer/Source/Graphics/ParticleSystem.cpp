#include "Pch.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Resource/FileLoader.h"

ParticleSystem::ParticleSystem()
{
	m_ParticlePool.resize(1000);
}

void ParticleSystem::Update(float deltaTime)
{
	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		if (particle.LifeRemaining <= 0.0f)
		{
			particle.Active = false;
			continue;
		}

		particle.LifeRemaining -= deltaTime;
		particle.Position += particle.Velocity * deltaTime;
		particle.Rotation += particle.BaseRotation * deltaTime;
	}
}

void ParticleSystem::Render()
{
	m_NumActiveParticles = 0;

	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		float life = particle.LifeRemaining / particle.LifeTime;
		glm::vec4 color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);
		color.a = color.a * life;

		float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);

		// Draw Quad
		m_NumActiveParticles++;
	}
}

void ParticleSystem::Emit(const ParticleProps& particleProps)
{
	Particle& particle = m_ParticlePool[m_PoolIndex];
	particle.Active = true;
	particle.Position = particleProps.Position;
	particle.Rotation = Random::FloatRange(1.0f, 2.0f) * glm::pi<float>();

	particle.Velocity = particleProps.Velocity;
	particle.Velocity.x += particleProps.VelocityVariation.x * Random::FloatRange(-0.5f, 0.5f);
	particle.Velocity.y += particleProps.VelocityVariation.y * Random::FloatRange(-0.5f, 0.5f);

	particle.ColorBegin = particleProps.ColorBegin;
	particle.ColorEnd = particleProps.ColorEnd;

	particle.LifeTime = particleProps.LifeTime;
	particle.LifeRemaining = particleProps.LifeTime;
	particle.BaseRotation = particleProps.BaseRotation + particleProps.BaseRotationVariation * Random::FloatRange(-0.5f, 0.5f);
	particle.SizeBegin = particleProps.SizeBegin + particleProps.SizeVariation * Random::FloatRange(-0.5f, 0.5f);
	particle.SizeEnd = particleProps.SizeEnd;

	m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
}
