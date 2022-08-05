#pragma once
#include "Scene/SceneObject.h"

class Model;

class ModelObject : public SceneObject
{
public:
	ModelObject(const std::shared_ptr<Model>& model, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~ModelObject();

	virtual void Update(float deltaTime);
	virtual void Render();

private:
	std::shared_ptr<Model> m_Model;

};
