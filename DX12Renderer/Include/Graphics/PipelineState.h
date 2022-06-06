#pragma once

class RootSignature;
class Shader;

class PipelineState
{
public:
	PipelineState(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath);
	~PipelineState();

	RootSignature* GetRootSignature() const { return m_RootSignature.get(); }
	ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const { return m_d3d12PipelineState; }
	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_d3d12PrimitiveToplogy; }
	
private:
	void Create(const std::wstring& vertexShaderPath, const std::wstring& pixelShaderPath);

private:
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY m_d3d12PrimitiveToplogy;

	std::unique_ptr<RootSignature> m_RootSignature;

};
