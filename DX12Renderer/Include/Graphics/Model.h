#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;

class Model
{
public:
	enum InputType : uint32_t
	{
		INPUT_POSITION,
		INPUT_TEX_COORD,
		INPUT_NORMAL,
		INPUT_INDEX,
		NUM_INPUT_TYPES
	};

	enum TextureType : uint32_t
	{
		TEX_ALBEDO,
		TEX_NORMAL,
		NUM_TEXTURE_TYPES
	};

public:
	Model(const tinygltf::Model& glTFModel, const std::string& name);
	~Model();

	const std::string& GetName() const { return m_Name; }
	std::shared_ptr<Buffer> GetBuffer(InputType type) const { return m_Buffers[type]; }
	std::shared_ptr<Texture> GetTexture(TextureType type) const { return m_Textures[type]; }

private:
	void CreateBuffers(const tinygltf::Model& glTFModel);
	void CreateTextures(const tinygltf::Model& glTFModel);

private:
	std::string m_Name = "";

	std::vector<std::shared_ptr<Buffer>> m_Buffers;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
