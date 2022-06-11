#pragma once
#include "Transform.h"

class Model;

class SceneObject
{
public:
	SceneObject(const std::shared_ptr<Model>& model, const glm::vec3& position, const glm::vec3& rotation, const glm::vec3& scale);
	~SceneObject();

	void Update(float deltaTime);
	void Render();

	const Transform& GetTransform() const { return m_Transform; }

private:
	Transform m_Transform;
	std::shared_ptr<Model> m_Model;

};
