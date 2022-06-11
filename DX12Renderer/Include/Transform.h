#pragma once

constexpr glm::vec3 WorldRight = glm::vec3(1.0f, 0.0f, 0.0f);
constexpr glm::vec3 WorldUp = glm::vec3(0.0f, 1.0f, 0.0f);
constexpr glm::vec3 WorldForward = glm::vec3(0.0f, 0.0f, 1.0f);

class Transform
{
public:
	Transform() = default;

	void Translate(const glm::vec3& position);
	void Rotate(const glm::vec3& rotation);
	void Scale(const glm::vec3& scale);

	/*void SetPosition(const glm::vec3& position);
	void SetRotation(const glm::vec3& rotation);
	void SetScale(const glm::vec3& scale);*/

	glm::vec3 Right();
	glm::vec3 Up();
	glm::vec3 Forward();

	const glm::mat4& GetTransformMatrix() const { return m_TransformMatrix; }
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetRotation() const { return m_Rotation; }
	const glm::vec3& GetScale() const { return m_Scale; }

private:
	glm::mat4 m_TransformMatrix = glm::identity<glm::mat4>();

	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Rotation = glm::vec3(0.0f);
	glm::fquat m_QuatRotation = glm::fquat();
	glm::vec3 m_Scale = glm::vec3(1.0f);

};
