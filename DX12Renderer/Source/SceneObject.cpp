#include "Pch.h"
#include "SceneObject.h"
#include "Graphics/Model.h"

#include "Application.h"
#include "Graphics/Renderer.h"

SceneObject::SceneObject(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale)
	: m_Model(model), m_Position(position), m_Rotation(rotation), m_Scale(scale)
{
	m_Transform = glm::identity<glm::mat4>();
	m_Transform = glm::translate(m_Transform, m_Position);
	m_Transform = glm::rotate(m_Transform, glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	m_Transform = glm::rotate(m_Transform, glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	m_Transform = glm::rotate(m_Transform, glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	m_Transform = glm::scale(m_Transform, m_Scale);
}

SceneObject::~SceneObject()
{
}

void SceneObject::Update(float deltaTime)
{
}

void SceneObject::Render()
{
	Application::Get().GetRenderer()->Submit(m_Model, m_Transform);
}
