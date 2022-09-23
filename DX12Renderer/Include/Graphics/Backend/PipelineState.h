#pragma once
#include "Graphics/Texture.h"
#include "Graphics/RenderPass.h"

class RootSignature;
class Shader;

class PipelineState
{
public:
	PipelineState(const std::string& name, const RenderPassDesc& renderPassDesc);
	~PipelineState();

	RootSignature* GetRootSignature() const { return m_RootSignature.get(); }
	ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const { return m_d3d12PipelineState; }

	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_d3d12PrimitiveToplogy; }
	
private:
	void Create(const std::string& name, const RenderPassDesc& renderPassDesc, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

private:
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY m_d3d12PrimitiveToplogy;

	std::unique_ptr<Shader> m_VertexShader;
	std::unique_ptr<Shader> m_PixelShader;
	std::unique_ptr<RootSignature> m_RootSignature;

};
