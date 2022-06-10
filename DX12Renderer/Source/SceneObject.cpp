#include "Pch.h"
#include "SceneObject.h"
#include "Graphics/Model.h"

#include "Application.h"
#include "Graphics/Renderer.h"

SceneObject::SceneObject(const std::shared_ptr<Model>& model, const glm::mat4& transform)
	: m_Model(model), m_Transform(transform)
{
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
