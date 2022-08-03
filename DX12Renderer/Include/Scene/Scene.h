#pragma once
#include "Graphics/ParticleSystem.h"
#include "Camera.h"

class SceneObject;

class Scene
{
public:
	Scene();
	~Scene();

	void Update(float deltaTime);
	void Render();
	void ImGuiRender();

	Camera& GetActiveCamera() { return m_ActiveCamera; }

private:
	Camera m_ActiveCamera;

	std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;

};
