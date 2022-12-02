#pragma once
#include "Transform.h"
#include "Scene/Camera/Camera.h"
#include "Components/Component.h"

class Model;
class Component;

class SceneObject
{
public:
	SceneObject(std::size_t id, const std::string& name);
	~SceneObject();

	void Update(float deltaTime);
	void Render();
	void OnImGuiRender();

	template<typename T, typename... TArgs>
	void AddComponent(TArgs&&... args)
	{
		uint32_t componentID = GetComponentID<T>();
		m_Components[componentID] = std::make_unique<T>(std::forward<TArgs>(args)...);
		m_Components[componentID]->SetObjectID(m_ID);

		m_ComponentBitFlag |= (1 << componentID);
	}

	template<typename T>
	void RemoveComponent()
	{
		uint32_t componentID = GetComponentID<T>();
		m_Components[componentID].release();
		m_ComponentBitFlag &= (0 << componentID);
	}

	template<typename T>
	T& GetComponent()
	{
		uint32_t componentID = GetComponentID<T>();
		return *static_cast<T*>(m_Components[componentID].get());
	}

	template<typename T>
	const T& GetComponent() const
	{
		uint32_t componentID = GetComponentID<T>();
		return *static_cast<const T*>(m_Components[componentID].get());
	}

	std::size_t GetID() const { return m_ID; }
	const std::string& GetName() const { return m_Name; }

private:
	std::size_t m_ID = 0;
	std::string m_Name = "Unnamed";

	// TODO: This shouldnt be a vector of components anymore.
	std::array<std::unique_ptr<Component>, 8> m_Components;
	uint8_t m_ComponentBitFlag = 0;

};
