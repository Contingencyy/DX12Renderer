#include "Pch.h"
#include "Transform.h"

Transform::Transform(const glm::mat4& transformMatrix)
	: m_TransformMatrix(transformMatrix)
{
	glm::vec3 skew;
	glm::vec4 perspective;

	glm::decompose(m_TransformMatrix, m_Scale, m_QuatRotation, m_Translation, skew, perspective);
	m_Rotation = glm::eulerAngles(m_QuatRotation);
}

void Transform::Translate(const glm::vec3& translation)
{
	m_Translation += translation;
	m_TranslationMatrix = glm::translate(glm::identity<glm::mat4>(), m_Translation);

	MakeTransformMatrix();
}

void Transform::Rotate(const glm::vec3& rotation)
{
	m_Rotation += rotation;
	m_QuatRotation = glm::fquat(m_Rotation);

	MakeTransformMatrix();
}

void Transform::Scale(const glm::vec3& scale)
{
	m_Scale *= scale;
	m_ScaleMatrix = glm::scale(glm::identity<glm::mat4>(), m_Scale);

	MakeTransformMatrix();
}

void Transform::SetTranslation(const glm::vec3& position)
{
	m_Translation = position;
	m_TranslationMatrix = glm::translate(glm::identity<glm::mat4>(), m_Translation);

	MakeTransformMatrix();
}

void Transform::SetRotation(const glm::vec3& rotation)
{
	m_Rotation = rotation;
	m_QuatRotation = glm::fquat(rotation);

	MakeTransformMatrix();
}

void Transform::SetScale(const glm::vec3& scale)
{
	m_Scale = scale;
	m_ScaleMatrix = glm::scale(glm::identity<glm::mat4>(), m_Scale);

	MakeTransformMatrix();
}

glm::vec3 Transform::Right() const
{
	return glm::vec3(m_TransformMatrix[0][0], m_TransformMatrix[1][0], m_TransformMatrix[2][0]);
}

glm::vec3 Transform::Up() const
{
	return glm::vec3(m_TransformMatrix[0][1], m_TransformMatrix[1][1], m_TransformMatrix[2][1]);
}

glm::vec3 Transform::Forward() const
{
	return glm::vec3(m_TransformMatrix[0][2], m_TransformMatrix[1][2], m_TransformMatrix[2][2]);
}

Transform Transform::GetInverse() const
{
	Transform inverseTransform(glm::inverse(m_TransformMatrix));
	return inverseTransform;
}

void Transform::MakeTransformMatrix()
{
	m_TransformMatrix = m_TranslationMatrix * glm::mat4_cast(m_QuatRotation) * m_ScaleMatrix;
}
