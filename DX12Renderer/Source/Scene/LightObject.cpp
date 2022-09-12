#include "Pch.h"
#include "Scene/LightObject.h"
#include "Graphics/Renderer.h"

/*

	Since distance attenuation is a quadratic function, we can calculate a light's range based on its constant, linear, and quadratic falloff.
	The light range epsilon defines the attenuation value that is required before the light source will be ignored for further calculations in the shaders.
	We calculate it once on the CPU so that the GPU does not have to do it for every fragment/pixel. Only applies to point and spotlights.
	In the shader we need to rescale the attenuation value to account for this cutoff.

*/

constexpr float LIGHT_RANGE_EPSILON = 1.0f / 0.01f;

DirectionalLightObject::DirectionalLightObject(const DirectionalLightData& dirLightData, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_DirectionalLightData(dirLightData)
{
	m_FrustumCullable = false;
}

DirectionalLightObject::~DirectionalLightObject()
{
}

void DirectionalLightObject::Update(float deltaTime)
{
}

void DirectionalLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_DirectionalLightData);
}

PointLightObject::PointLightObject(const PointLightData& pointLightData, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_PointLightData(pointLightData)
{
	m_FrustumCullable = false;
	m_PointLightData.Range = MathHelper::SolveQuadraticFunc(m_PointLightData.Attenuation.z, m_PointLightData.Attenuation.y, m_PointLightData.Attenuation.x - LIGHT_RANGE_EPSILON);
}

PointLightObject::~PointLightObject()
{
}

void PointLightObject::Update(float deltaTime)
{
	m_PointLightData.Position = m_Transform.GetPosition();
}

void PointLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_PointLightData);
}

SpotLightObject::SpotLightObject(const SpotLightData& spotLightData, const std::string& name, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_SpotLightData(spotLightData)
{
	m_FrustumCullable = false;
	m_SpotLightData.Range = MathHelper::SolveQuadraticFunc(m_SpotLightData.Attenuation.z, m_SpotLightData.Attenuation.y, m_SpotLightData.Attenuation.x - LIGHT_RANGE_EPSILON);
}

SpotLightObject::~SpotLightObject()
{
}

void SpotLightObject::Update(float deltaTime)
{
	m_SpotLightData.Position = m_Transform.GetPosition();
}

void SpotLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_SpotLightData);
}
