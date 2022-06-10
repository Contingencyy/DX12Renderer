#pragma once

class Model;

class SceneObject
{
public:
	SceneObject(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
	~SceneObject();

	void Update(float deltaTime);
	void Render();

	const glm::mat4& GetTransform() const { return m_Transform; }
	const glm::vec3& GetPosition() const { return m_Position; }
	const glm::vec3& GetRotation() const { return m_Rotation; }
	const glm::vec3& GetScale() const { return m_Scale; }

private:
	std::shared_ptr<Model> m_Model;

	glm::mat4 m_Transform = glm::identity<glm::mat4>();
	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Rotation = glm::vec3(0.0f);
	glm::vec3 m_Scale = glm::vec3(1.0f);

};
