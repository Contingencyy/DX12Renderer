#include "Pch.h"
#include "Transform.h"

void Transform::Translate(const glm::vec3& position)
{
	m_Position += position;
	m_TransformMatrix = glm::translate(m_TransformMatrix, position);
}

void Transform::Rotate(const glm::vec3& rotation)
{
	m_Rotation += rotation;

	m_QuatRotation = glm::angleAxis(glm::radians(rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
	m_TransformMatrix = m_TransformMatrix * glm::mat4_cast(m_QuatRotation);
	m_QuatRotation = glm::angleAxis(glm::radians(rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
	m_TransformMatrix = m_TransformMatrix * glm::mat4_cast(m_QuatRotation);
	m_QuatRotation = glm::angleAxis(glm::radians(rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
	m_TransformMatrix = m_TransformMatrix * glm::mat4_cast(m_QuatRotation);
}

void Transform::Scale(const glm::vec3& scale)
{
	m_Scale *= scale;
	m_TransformMatrix = glm::scale(m_TransformMatrix, scale);
}

//void Transform::SetPosition(const glm::vec3& position)
//{
//	m_Position = position;
//	m_TransformMatrix = glm::translate(glm::identity<glm::mat4>(), m_Position);
//}
//
//void Transform::SetRotation(const glm::vec3& rotation)
//{
//	m_Rotation = rotation;
//
//	m_QuatRotation = glm::angleAxis(glm::radians(m_Rotation.x), glm::vec3(1.0f, 0.0f, 0.0f));
//	m_QuatRotation = glm::angleAxis(glm::radians(m_Rotation.y), glm::vec3(0.0f, 1.0f, 0.0f));
//	m_QuatRotation = glm::angleAxis(glm::radians(m_Rotation.z), glm::vec3(0.0f, 0.0f, 1.0f));
//
//	m_TransformMatrix = glm::identity<glm::mat4>() * glm::mat4_cast(m_QuatRotation);
//}
//
//void Transform::SetScale(const glm::vec3& scale)
//{
//	m_Scale = scale;
//	m_TransformMatrix = m_TransformMatrix * glm::scale(glm::identity<glm::mat4>(), m_Scale);
//}

glm::vec3 Transform::Right()
{
	return glm::vec3(m_TransformMatrix[0][0], m_TransformMatrix[1][0], m_TransformMatrix[2][0]);
}

glm::vec3 Transform::Up()
{
	return glm::vec3(m_TransformMatrix[0][1], m_TransformMatrix[1][1], m_TransformMatrix[2][1]);
}

glm::vec3 Transform::Forward()
{
	return glm::vec3(m_TransformMatrix[0][2], m_TransformMatrix[1][2], m_TransformMatrix[2][2]);
}
