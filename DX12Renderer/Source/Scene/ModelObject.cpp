#include "Pch.h"
#include "Scene/ModelObject.h"

#include "Application.h"
#include "Graphics/Renderer.h"

ModelObject::ModelObject(const std::shared_ptr<Model>& model, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_Model(model)
{
}

ModelObject::~ModelObject()
{
}

void ModelObject::Update(float deltaTime)
{
}

void ModelObject::Render()
{
	Application::Get().GetRenderer()->Submit(m_Model, m_Transform.GetTransformMatrix());
}
