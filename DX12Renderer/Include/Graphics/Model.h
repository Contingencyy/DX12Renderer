#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;
class PipelineState;
class RootSignature;

class Model
{
public:
	Model(const std::string& filepath);
	~Model();

	void Render();

	PipelineState* GetPipelineState() const { return m_PipelineState.get(); }

	std::shared_ptr<Buffer> GetBuffer(uint32_t index) const { return m_Buffers[index]; }
	std::shared_ptr<Texture> GetTexture(uint32_t index) const { return m_Textures[index]; }

private:
	void CreatePipelineState();

	void CreateBuffers(tinygltf::Model* glTFModel);
	void CreateTextures(tinygltf::Model* glTFModel);

private:
	std::unique_ptr<PipelineState> m_PipelineState;

	std::vector<std::shared_ptr<Buffer>> m_Buffers;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
