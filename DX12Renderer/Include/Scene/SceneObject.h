#pragma once
#include "Transform.h"
#include "Scene/Camera/Camera.h"
#include "Components/Component.h"

class Model;
class Component;

class SceneObject
{
public:
	SceneObject(const std::string& name, const glm::vec3& translation = glm::vec3(0.0f),
		const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~SceneObject();

	void Update(float deltaTime);
	void Render();
	void OnImGuiRender();

	template<typename T, typename... TArgs>
	void AddComponent(TArgs&&... args)
	{
		uint32_t componentID = GetComponentID<T>();
		m_Components[componentID].emplace_back(std::make_unique<T>(std::forward<TArgs>(args)...));
	}

	template<typename T>
	void RemoveComponent(uint32_t index)
	{
		uint32_t componentID = GetComponentID<T>();
		m_Components[componentID].erase(index);
	}

	template<typename T>
	const T& GetComponent(uint32_t index)
	{
		uint32_t componentID = GetComponentID<T>();
		return *m_Components[componentID][index];
	}

	const Transform& GetTransform() const { return m_Transform; }
	const std::string& GetName() const { return m_Name; }

private:
	Transform m_Transform;
	std::string m_Name;

	std::array<std::vector<std::unique_ptr<Component>>, 8> m_Components;

};
