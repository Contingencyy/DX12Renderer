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

	ComPtr<ID3D12RootSignature> GetRootSignature() const { return m_d3d12RootSignature; }
	ComPtr<ID3D12PipelineState> GetPipelineState() const { return m_d3d12PipelineState; }

	std::shared_ptr<Buffer> GetBuffer(uint32_t index) const { return m_Buffers[index]; }
	std::shared_ptr<Texture> GetTexture(uint32_t index) const { return m_Textures[index]; }

private:
	void CreateRootSignature();
	void CreatePipelineState();
	void CreateBuffers();
	void CreateTextures();

private:
	tinygltf::Model m_TinyglTFModel;

	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;

	std::vector<std::shared_ptr<Buffer>> m_Buffers;
	std::vector<std::shared_ptr<Texture>> m_Textures;

};
