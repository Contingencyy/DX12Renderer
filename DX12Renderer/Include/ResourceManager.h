#pragma once

class Texture;
class Model;

class ResourceManager
{
public:
	ResourceManager();
	~ResourceManager();

	void LoadTexture(const std::string& filepath, const std::string& name);
	void LoadModel(const std::string& filepath, const std::string& name);

	std::shared_ptr<Texture> GetTexture(const std::string& name);
	std::shared_ptr<Model> GetModel(const std::string& name);

private:
	std::unordered_map<std::string, std::shared_ptr<Texture>> m_Textures;
	std::unordered_map<std::string, std::shared_ptr<Model>> m_Models;

};
