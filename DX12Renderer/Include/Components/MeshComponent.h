#pragma once
#include "Component.h"

class Mesh;

class MeshComponent : public Component
{
public:
	MeshComponent(const std::shared_ptr<Mesh>& mesh);
	virtual ~MeshComponent();

	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void OnImGuiRender();

private:
	std::shared_ptr<Mesh> m_Mesh;

};
