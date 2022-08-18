#pragma once

constexpr glm::vec3 WorldRight = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 WorldForward = glm::vec3(0.0f, 0.0f, 1.0f);

class Transform
{
public:
	Transform() = default;
	Transform(const glm::mat4& transformMatrix);

	void Translate(const glm::vec3& translation);
	void Rotate(const glm::vec3& rotation);
	void Scale(const glm::vec3& scale);

	glm::vec3 Right() const;
	glm::vec3 Up() const;
	glm::vec3 Forward() const;

	const glm::mat4& GetTransformMatrix() const { return m_TransformMatrix; }
	Transform GetInverse() const;

	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetRotation() const { return m_Rotation; }
	const glm::vec3& GetScale() const { return m_Scale; }

private:
	void MakeTransformMatrix();

private:
	glm::mat4 m_TransformMatrix = glm::identity<glm::mat4>();

	glm::mat4 m_TranslationMatrix = glm::identity<glm::mat4>();
	glm::fquat m_QuatRotation = glm::fquat();
	glm::mat4 m_ScaleMatrix = glm::identity<glm::mat4>();

	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Rotation = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);

};
