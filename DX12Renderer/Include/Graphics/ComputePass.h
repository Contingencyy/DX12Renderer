#pragma once

class Shader;

struct ComputePassDesc
{
	std::string ComputeShaderPath;

	uint32_t NumThreadsX;
	uint32_t NumThreadsY;
	uint32_t NumThreadsZ;

	std::vector<CD3DX12_ROOT_PARAMETER1> RootParameters;
};

class ComputePass
{
public:
	ComputePass(const std::string& name, const ComputePassDesc& desc);

	ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const { return m_d3d12PipelineState; }
	ComPtr<ID3D12RootSignature> GetD3D12RootSignature() const { return m_d3d12RootSignature; }

	const ComputePassDesc& GetDesc() const { return m_Desc; }

private:
	void CreateRootSignature();
	void CreatePipelineState();

private:
	ComputePassDesc m_Desc;
	std::string m_Name;

	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;

	std::unique_ptr<Shader> m_ComputeShader;

};
