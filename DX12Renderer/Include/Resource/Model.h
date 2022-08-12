#pragma once
#include <tinygltf/tiny_gltf.h>

class Mesh;
class Texture;

class Model
{
public:
	Model(const tinygltf::Model& glTFModel, const std::string& name);
	~Model();

	const std::string& GetName() const { return m_Name; }
	const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const { return m_Meshes; }

private:
	void CreateMeshes(const tinygltf::Model& glTFModel);

private:
	std::string m_Name = "";

	std::vector<std::shared_ptr<Mesh>> m_Meshes;

};
