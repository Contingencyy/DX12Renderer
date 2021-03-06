#pragma once

struct ParticleProps
{
	glm::vec2 Position;
	glm::vec2 Velocity, VelocityVariation;
	glm::vec4 ColorBegin, ColorEnd;

	float SizeBegin, SizeEnd, SizeVariation;
	float BaseRotation, BaseRotationVariation;
	float LifeTime = 1.0f;
};

class Buffer;
class Texture;

class ParticleSystem
{
public:
	ParticleSystem();

	void Update(float deltaTime);
	void Render();

	void Emit(const ParticleProps& particleProps);

	uint32_t GetNumActiveParticles() const { return m_NumActiveParticles; }

private:
	struct Particle
	{
		glm::vec2 Position;
		glm::vec2 Velocity;
		glm::vec4 ColorBegin, ColorEnd;

		float Rotation = 0.0f, BaseRotation;
		float SizeBegin, SizeEnd;

		float LifeTime = 1.0f;
		float LifeRemaining = 0.0f;

		bool Active = false;
	};

	std::vector<Particle> m_ParticlePool;
	uint32_t m_PoolIndex = 999;
	uint32_t m_NumActiveParticles = 0;

};
