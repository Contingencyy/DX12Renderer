#pragma once

class Camera;
class Transform;

using ComponentID = uint32_t;

inline ComponentID GetUniqueComponentID()
{
	static ComponentID lastID = 0;
	return lastID++;
}

template<typename T>
inline ComponentID GetComponentID()
{
	static ComponentID typeID = GetUniqueComponentID();
	return typeID;
}

class Component
{
public:
	Component();
	virtual ~Component();

	virtual void Update(float deltaTime) = 0;
	virtual void Render(const Transform& transform) = 0;
	virtual void OnImGuiRender() = 0;

};
