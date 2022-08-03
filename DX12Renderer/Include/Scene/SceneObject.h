#pragma once
#include "Transform.h"

class Model;

class SceneObject
{
public:
	SceneObject(const std::string& name, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale);
	~SceneObject();

	virtual void Update(float deltaTime) = 0;
	virtual void Render() = 0;

	const Transform& GetTransform() const { return m_Transform; }
	const std::string& GetName() const { return m_Name; }

protected:
	Transform m_Transform;
	std::string m_Name;

};
