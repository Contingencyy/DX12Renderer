#pragma once

class Model;

class SceneObject
{
public:
	SceneObject(const std::shared_ptr<Model>& model, const glm::mat4& transform);
	~SceneObject();

	void Update(float deltaTime);
	void Render();

private:
	std::shared_ptr<Model> m_Model;

	glm::mat4 m_Transform = glm::identity<glm::mat4>();

};
