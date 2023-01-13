#pragma once
#include "Graphics/Texture.h"

class FrameBuffer;
class Shader;

struct RasterPassDesc
{
	std::string VertexShaderPath;
	std::string PixelShaderPath;

	TextureDesc ColorAttachmentDesc;
	TextureDesc DepthAttachmentDesc;
	
	bool DepthEnabled = true;
	D3D12_COMPARISON_FUNC DepthComparisonFunc = D3D12_COMPARISON_FUNC_GREATER;
	int DepthBias = 0;
	float SlopeScaledDepthBias = 0.0f;
	float DepthBiasClamp = 1.0f;

	D3D12_PRIMITIVE_TOPOLOGY Topology = D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

	std::vector<CD3DX12_ROOT_PARAMETER1> RootParameters;
	std::vector<D3D12_INPUT_ELEMENT_DESC> ShaderInputLayout;
};

class RasterPass
{
public:
	RasterPass(const std::string& name, const RasterPassDesc& desc);

	ID3D12PipelineState* GetD3D12PipelineState() const { return m_d3d12PipelineState.Get(); }
	ID3D12RootSignature* GetD3D12RootSignature() const { return m_d3d12RootSignature.Get(); }

	const RasterPassDesc& GetDesc() const { return m_Desc; }

private:
	void CreateRootSignature();
	void CreatePipelineState();

private:
	RasterPassDesc m_Desc;
	std::string m_Name;

	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;

	std::unique_ptr<Shader> m_VertexShader;
	std::unique_ptr<Shader> m_PixelShader;

};
