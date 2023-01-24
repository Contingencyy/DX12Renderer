#pragma once
#include "Graphics/RenderAPI.h"

class Texture;

struct Model
{
	struct Node
	{
		std::vector<RenderResourceHandle> MeshHandles;
		glm::mat4 Transform;
		std::string Name;

		std::vector<std::size_t> Children;
	};

	std::vector<Node> Nodes;
	std::vector<std::size_t> RootNodes;
	std::string Name = "";
};

namespace ResourceManager
{

	void LoadTexture(const std::string& filepath, const std::string& name);
	void LoadModel(const std::string& filepath, const std::string& name);

	std::shared_ptr<Texture> GetTexture(const std::string& name);
	std::shared_ptr<Model> GetModel(const std::string& name);

};
