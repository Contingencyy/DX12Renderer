#pragma once
#include "Graphics/ParticleSystem.h"
#include "Scene/Camera/Camera.h"

class SceneObject;

class Scene
{
public:
	Scene();

	void Update(float deltaTime);
	void Render();
	void OnImGuiRender();

	static std::size_t AddSceneObject(const std::string& name);
	static SceneObject& GetSceneObject(std::size_t objectID);
	static void SpawnModelObject(const std::string& modelName, const glm::vec3& translation = glm::vec3(0.0f),
		const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));

	Camera& GetActiveCamera() { return m_ActiveCamera; }
	const Camera& GetActiveCamera() const { return m_ActiveCamera; }

private:
	Camera m_ActiveCamera;

};
