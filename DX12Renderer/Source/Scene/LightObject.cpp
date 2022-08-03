#include "Pch.h"
#include "Scene/LightObject.h"

#include "Application.h"
#include "Graphics/Renderer.h"

PointlightObject::PointlightObject(const PointlightData& pointlightData, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_PointlightData(pointlightData)
{
}

PointlightObject::~PointlightObject()
{
}

void PointlightObject::Update(float deltaTime)
{
	m_PointlightData.Position = m_Transform.GetPosition();
}

void PointlightObject::Render()
{
	Application::Get().GetRenderer()->Submit(m_PointlightData);
}
