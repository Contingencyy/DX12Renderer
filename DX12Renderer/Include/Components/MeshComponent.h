#pragma once
#include "Component.h"
#include "Scene/BoundingVolume.h"
#include "Graphics/RenderAPI.h"

class MeshComponent : public Component
{
public:
	MeshComponent(RenderResourceHandle meshHandle);
	virtual ~MeshComponent();

	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void OnImGuiRender();

private:
	RenderResourceHandle m_Mesh;

};
