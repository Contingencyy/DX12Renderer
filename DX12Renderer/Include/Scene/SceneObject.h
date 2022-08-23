#pragma once
#include "Transform.h"
#include "Scene/Camera/Camera.h"

class Model;

class SceneObject
{
public:
	SceneObject(const std::string& name, const glm::vec3& translation = glm::vec3(0.0f),
		const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~SceneObject();

	virtual void Update(float deltaTime) = 0;
	virtual void Render(const Camera& camera) = 0;

	const Transform& GetTransform() const { return m_Transform; }
	const std::string& GetName() const { return m_Name; }
	bool IsFrustumCullable() const { return m_FrustumCullable; }

protected:
	Transform m_Transform;
	std::string m_Name;

	bool m_FrustumCullable = true;

};
