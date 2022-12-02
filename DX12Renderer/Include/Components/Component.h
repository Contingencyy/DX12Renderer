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
	virtual void Render() = 0;
	virtual void OnImGuiRender() = 0;

	void SetObjectID(std::size_t id) { m_ObjectID = id; }
	
protected:
	std::size_t m_ObjectID = UINT64_MAX;

};
