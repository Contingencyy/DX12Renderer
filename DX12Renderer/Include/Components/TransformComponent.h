#pragma once
#include "Component.h"
#include "Transform.h"

class TransformComponent : public Component
{
public:
	TransformComponent(const glm::vec3& translation = glm::vec3(0.0f),
		const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	virtual ~TransformComponent();

	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void OnImGuiRender();

	Transform& GetTransform() { return m_Transform; }
	const Transform& GetTransform() const { return m_Transform; }

private:
	Transform m_Transform;

};
