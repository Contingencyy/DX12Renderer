#pragma once
#include "Scene/SceneObject.h"

class Mesh;

class MeshObject : public SceneObject
{
public:
	MeshObject(const std::vector<std::shared_ptr<Mesh>>& meshes, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~MeshObject();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera);

private:
	std::vector<std::shared_ptr<Mesh>> m_Meshes;

};
