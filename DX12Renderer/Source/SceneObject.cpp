#include "Pch.h"
#include "SceneObject.h"
#include "Graphics/Model.h"

#include "Application.h"
#include "Graphics/Renderer.h"

SceneObject::SceneObject(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
	: m_Model(model)
{
	m_Transform.Translate(position);
	m_Transform.Rotate(rotation);
	m_Transform.Scale(scale);
}

SceneObject::~SceneObject()
{
}

void SceneObject::Update(float deltaTime)
{
}

void SceneObject::Render()
{
	Application::Get().GetRenderer()->Submit(m_Model, m_Transform.GetTransformMatrix());
}
