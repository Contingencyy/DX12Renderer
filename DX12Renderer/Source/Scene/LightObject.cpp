#include "Pch.h"
#include "Scene/LightObject.h"
#include "Graphics/Renderer.h"

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
	//m_SpotLightData.Position = camera.GetTransform().GetPosition();
	//m_SpotLightData.Direction = glm::vec3(camera.GetTransform().GetInverse().Forward());
	Renderer::Submit(m_SpotLightData);
}
