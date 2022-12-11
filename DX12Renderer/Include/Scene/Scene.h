#pragma once
#include "Graphics/ParticleSystem.h"
#include "Scene/Camera/Camera.h"

class SceneObject;

class Scene
{
public:
	Scene();
	~Scene();

	void Update(float deltaTime);
	void Render();
	void OnImGuiRender();

	static std::size_t AddSceneObject(const std::string& name);
	static SceneObject& GetSceneObject(std::size_t objectID);

	Camera& GetActiveCamera() { return m_ActiveCamera; }

private:
	Camera m_ActiveCamera;

};
