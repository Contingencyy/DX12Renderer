#include "Pch.h"
#include "Scene/SceneObject.h"
#include "Resource/Model.h"

#include "Application.h"
#include "Graphics/Renderer.h"

SceneObject::SceneObject(const std::string& name, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: m_Name(name)
{
	m_Transform.Translate(translation);
	m_Transform.Rotate(rotation);
	m_Transform.Scale(scale);
}

SceneObject::~SceneObject()
{
}
