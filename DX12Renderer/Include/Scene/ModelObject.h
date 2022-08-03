#pragma once
#include "Scene/SceneObject.h"

class Model;

class ModelObject : public SceneObject
{
public:
	ModelObject(const std::shared_ptr<Model>& model, const std::string& name,
		const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);
	~ModelObject();

	virtual void Update(float deltaTime);
	virtual void Render();

private:
	std::shared_ptr<Model> m_Model;

};
