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
		const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f), bool frustumCullable = false);
	~SceneObject();

	void Update(float deltaTime);
	void Render(const Camera& camera);
	void OnImGuiRender();

	template<typename T, typename... TArgs>
	void AddComponent(TArgs&&... args)
	{
		uint32_t componentID = GetComponentID<T>();

		m_Components[componentID] = std::make_unique<T>(std::forward<TArgs>(args)...);
		m_ComponentBitFlag |= (1 << componentID);
	}

	template<typename T>
	void RemoveComponent()
	{
		uint32_t componentID = GetComponentID<T>();

		m_Components[componentID].reset(nullptr);
		m_ComponentBitFlag &= (0 << componentID);
	}

	template<typename T>
	bool HasComponent() const
	{
		return m_ComponentBitFlag & (1 << GetComponentID<T>());
	}

	template<typename T>
	const T& GetComponent() const
	{
		uint32_t componentID = GetComponentID<T>();
		return m_Components[componentID];
	}

	const Transform& GetTransform() const { return m_Transform; }
	const std::string& GetName() const { return m_Name; }
	bool IsFrustumCullable() const { return m_FrustumCullable; }

private:
	Transform m_Transform;
	std::string m_Name;

	std::array<std::unique_ptr<Component>, 8> m_Components;
	uint8_t m_ComponentBitFlag = 0;

	bool m_FrustumCullable = true;

};
