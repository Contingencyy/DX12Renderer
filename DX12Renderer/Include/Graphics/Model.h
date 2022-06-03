#pragma once
#include <tinygltf/tiny_gltf.h>

class Buffer;
class Texture;

class Model
{
public:
	Model(const std::string& filepath);
	~Model();

	void Render();

private:
	void CreateRootSignature();
	void CreatePipelineState();
	void CreateBuffers();
	void CreateTextures();

private:
	friend class Renderer;

	tinygltf::Model m_TinyglTFModel;

	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;

	std::vector<std::shared_ptr<Buffer>> m_Buffers;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
