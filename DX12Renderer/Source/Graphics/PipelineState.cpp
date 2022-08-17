#include "Pch.h"
#include "Graphics/PipelineState.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/RootSignature.h"
#include "Graphics/Shader.h"

PipelineState::PipelineState(const PipelineStateDesc& pipelineStateDesc, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
	const std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters, bool lineTopology)
	: m_PipelineStateDesc(pipelineStateDesc), m_LineTopology(lineTopology)
{
	m_RootSignature = std::make_unique<RootSignature>(descriptorRanges, rootParameters);
	Create(inputLayout);
}

PipelineState::~PipelineState()
{
}

void PipelineState::Create(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout)
{
	m_VertexShader = std::make_unique<Shader>(m_PipelineStateDesc.VertexShaderPath, "main", "vs_5_1");
	m_PixelShader = std::make_unique<Shader>(m_PipelineStateDesc.PixelShaderPath, "main", "ps_5_1");

	m_ColorAttachment = std::make_unique<Texture>(m_PipelineStateDesc.ColorAttachmentDesc);
	m_DepthStencilAttachment = std::make_unique<Texture>(m_PipelineStateDesc.DepthStencilAttachmentDesc);

	D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = TRUE;
	rtBlendDesc.LogicOpEnable = FALSE;
	rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0] = rtBlendDesc;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { &inputLayout[0], static_cast<uint32_t>(inputLayout.size()) };
	psoDesc.VS = m_VertexShader->GetShaderByteCode();
	psoDesc.PS = m_PixelShader->GetShaderByteCode();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = m_PipelineStateDesc.DepthTest ? TRUE : FALSE;
	psoDesc.DSVFormat = TextureFormatToDXGI(m_DepthStencilAttachment->GetTextureDesc().Format);
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = m_LineTopology ? D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE : D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = TextureFormatToDXGI(m_ColorAttachment->GetTextureDesc().Format);
	psoDesc.SampleDesc.Count = 1;
	psoDesc.pRootSignature = m_RootSignature->GetD3D12RootSignature().Get();

	Application::Get().GetRenderer()->GetDevice()->CreatePipelineState(psoDesc, m_d3d12PipelineState);

	m_d3d12PrimitiveToplogy = m_LineTopology ? D3D10_PRIMITIVE_TOPOLOGY_LINELIST : D3D10_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
}
