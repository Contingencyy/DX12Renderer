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

	Camera& GetActiveCamera() { return m_ActiveCamera; }
	const glm::vec3& GetAmbientLight() const { return m_AmbientLight; }

private:
	Camera m_ActiveCamera;
	glm::vec3 m_AmbientLight = glm::vec3(0.0f);

	std::vector<std::unique_ptr<SceneObject>> m_SceneObjects;

};
