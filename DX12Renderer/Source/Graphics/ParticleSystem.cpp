#include "Pch.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Buffer.h"
#include "Application.h"
#include "Renderer.h"

ParticleSystem::ParticleSystem()
{
	m_ParticlePool.resize(1000);
	m_ParticleInstanceData.reserve(1000);

	m_QuadInstanceDataBuffer = std::make_shared<Buffer>(BufferDesc(), m_ParticlePool.size(), sizeof(ParticleInstanceData));
	m_UploadBuffer = std::make_shared<Buffer>(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadInstanceDataBuffer->GetAlignedSize());
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
	m_ParticleInstanceData.clear();

	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		// Lerp particle values based on life remaining
		float life = particle.LifeRemaining / particle.LifeTime;
		glm::vec4 color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);
		color.a = color.a * life;

		float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);

		// Set particle instance data
		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), { particle.Position.x, particle.Position.y, 0.0f })
			* glm::rotate(glm::identity<glm::mat4>(), particle.Rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::identity<glm::mat4>(), { size, size, 1.0f });
		m_ParticleInstanceData.emplace_back(transform, color);
	}

	// Render
	auto renderer = Application::Get().GetRenderer();
	if (m_ParticleInstanceData.size() > 0)
		renderer->CopyBuffer(*m_UploadBuffer, *m_QuadInstanceDataBuffer, &m_ParticleInstanceData[0]);

	renderer->DrawQuads(m_QuadInstanceDataBuffer, m_ParticleInstanceData.size());

	m_NumActiveParticles = m_ParticleInstanceData.size();
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
