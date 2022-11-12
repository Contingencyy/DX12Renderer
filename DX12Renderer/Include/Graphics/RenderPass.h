#pragma once
#include "Graphics/Texture.h"

class FrameBuffer;
class PipelineState;

struct RenderPassDesc
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

class RenderPass
{
public:
	RenderPass(const std::string& name, const RenderPassDesc& desc);
	~RenderPass();

	RenderPassDesc& GetRenderPassDesc() { return m_RenderPassDesc; }
	const RenderPassDesc& GetRenderPassDesc() const { return m_RenderPassDesc; }
	const PipelineState& GetPipelineState() const { return *m_PipelineState; }

private:
	RenderPassDesc m_RenderPassDesc;
	std::string m_Name;

	std::unique_ptr<PipelineState> m_PipelineState;

};
