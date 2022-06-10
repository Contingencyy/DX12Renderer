#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;
class PipelineState;
class RootSignature;

class Model
{
public:
	Model(const tinygltf::Model& glTFModel, const std::string& name);
	~Model();

	const std::string& GetName() const { return m_Name; }

	PipelineState* GetPipelineState() const { return m_PipelineState.get(); }

	std::shared_ptr<Buffer> GetBuffer(uint32_t index) const { return m_Buffers[index]; }
	std::shared_ptr<Texture> GetTexture(uint32_t index) const { return m_Textures[index]; }

private:
	void CreatePipelineState();

	void CreateBuffers(const tinygltf::Model& glTFModel);
	void CreateTextures(const tinygltf::Model& glTFModel);

private:
	std::string m_Name = "";

	std::unique_ptr<PipelineState> m_PipelineState;

	std::vector<std::shared_ptr<Buffer>> m_Buffers;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
