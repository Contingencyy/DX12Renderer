#pragma once

class Texture;
class Vertex;
class Mesh;
struct RenderResourceHandle;

struct Model
{
	Model(const std::vector<RenderResourceHandle>& meshes, const std::string& name)
		: Meshes(meshes), Name(name) {}

	std::vector<RenderResourceHandle> Meshes;
	std::string Name = "";
};

class ResourceManager
{
public:
	ResourceManager();

	void LoadTexture(const std::string& filepath, const std::string& name);
	void LoadModel(const std::string& filepath, const std::string& name);

	std::shared_ptr<Texture> GetTexture(const std::string& name);
	std::shared_ptr<Model> GetModel(const std::string& name);

private:
	void CalculateVertexTangents(Vertex& vert0, Vertex& vert1, Vertex& vert2);

private:
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;

};
