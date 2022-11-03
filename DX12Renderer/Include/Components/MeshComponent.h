#pragma once
#include "Component.h"

class Mesh;

class MeshComponent : public Component
{
public:
	MeshComponent(const std::vector<std::shared_ptr<Mesh>>& meshes);
	virtual ~MeshComponent();

	virtual void Update(float deltaTime);
	virtual void Render(const Transform& transform);
	virtual void OnImGuiRender();

private:
	std::vector<std::shared_ptr<Mesh>> m_Meshes;

};
