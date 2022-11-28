#include "Pch.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Shader.h"
#include "Graphics/Backend/RenderBackend.h"

RenderPass::RenderPass(const std::string& name, const RenderPassDesc& desc)
	: m_Name(name), m_RenderPassDesc(desc)
{
	CreateRootSignature();
	CreatePipelineState();
}

RenderPass::~RenderPass()
{
}

void RenderPass::CreateRootSignature()
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	// Default texture sampler (antisotropic)
	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[2] = {};
	staticSamplers[0].Filter = D3D12_FILTER_ANISOTROPIC;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MaxAnisotropy = 8;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	// Force shader to use a lower/higher mip (e.g. if the shader was going to sample from LOD3, and MipLODBias is 1, it will instead sample from LOD4)
	staticSamplers[0].MipLODBias = 0;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Shadow map sampler
	// This sampler performs the percentage-closer filtering in hardware
	staticSamplers[1].Filter = D3D12_FILTER_COMPARISON_MIN_MAG_MIP_LINEAR;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].MaxAnisotropy = 1;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplers[1].MinLOD = 0.0f;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].MipLODBias = 0;
	staticSamplers[1].ShaderRegister = 0;
	staticSamplers[1].RegisterSpace = 1;
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(static_cast<uint32_t>(m_RenderPassDesc.RootParameters.size()),
		&m_RenderPassDesc.RootParameters[0],_countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &serializedRootSig, &errorBlob);
	if (!SUCCEEDED(hr) || errorBlob)
	{
		LOG_ERR(static_cast<const char*>(errorBlob->GetBufferPointer()));
		errorBlob->Release();
	}

	DX_CALL(RenderBackend::GetD3D12Device()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_d3d12RootSignature)));
	m_d3d12RootSignature->SetName(StringHelper::StringToWString(m_Name + " root signature").c_str());
}

void RenderPass::CreatePipelineState()
{
	m_VertexShader = std::make_unique<Shader>(StringHelper::StringToWString(m_RenderPassDesc.VertexShaderPath), "main", "vs_6_0");
	m_PixelShader = std::make_unique<Shader>(StringHelper::StringToWString(m_RenderPassDesc.PixelShaderPath), "main", "ps_6_0");

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
	psoDesc.InputLayout = { &m_RenderPassDesc.ShaderInputLayout[0], static_cast<uint32_t>(m_RenderPassDesc.ShaderInputLayout.size()) };
	psoDesc.VS = m_VertexShader->GetShaderByteCode();
	psoDesc.PS = m_PixelShader->GetShaderByteCode();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = m_RenderPassDesc.DepthEnabled ? TRUE : FALSE;

	if (m_RenderPassDesc.DepthEnabled)
	{
		psoDesc.DSVFormat = TextureFormatToDXGIFormat(m_RenderPassDesc.DepthAttachmentDesc.Format);
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
		psoDesc.DepthStencilState.DepthFunc = m_RenderPassDesc.DepthComparisonFunc;
		psoDesc.RasterizerState.DepthBias = m_RenderPassDesc.DepthBias;
		psoDesc.RasterizerState.SlopeScaledDepthBias = m_RenderPassDesc.SlopeScaledDepthBias;
		psoDesc.RasterizerState.DepthBiasClamp = m_RenderPassDesc.DepthBiasClamp;
	}

	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = m_RenderPassDesc.TopologyType;

	if (m_RenderPassDesc.ColorAttachmentDesc.Usage == TextureUsage::TEXTURE_USAGE_NONE && m_RenderPassDesc.ColorAttachmentDesc.Format == TextureFormat::TEXTURE_FORMAT_UNSPECIFIED)
	{
		psoDesc.NumRenderTargets = 0;
	}
	else
	{
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = TextureFormatToDXGIFormat(m_RenderPassDesc.ColorAttachmentDesc.Format);
	}

	psoDesc.SampleDesc.Count = 1;
	psoDesc.pRootSignature = m_d3d12RootSignature.Get();

	DX_CALL(RenderBackend::GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_d3d12PipelineState)));
	m_d3d12PipelineState->SetName(StringHelper::StringToWString(m_Name + " pipeline state").c_str());
}
