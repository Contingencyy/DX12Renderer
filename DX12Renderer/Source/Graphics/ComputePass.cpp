#include "Pch.h"
#include "Graphics/ComputePass.h"
#include "Graphics/Shader.h"
#include "Graphics/Backend/RenderBackend.h"

ComputePass::ComputePass(const std::string& name, const ComputePassDesc& desc)
	: m_Name(name), m_Desc(desc)
{
	CreateRootSignature();
	CreatePipelineState();
}

void ComputePass::CreateRootSignature()
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	// Default sampler (point)
	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[0].MaxAnisotropy = 1;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].MipLODBias = 0;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(static_cast<uint32_t>(m_Desc.RootParameters.size()),
		&m_Desc.RootParameters[0], 1, staticSamplers, rootSignatureFlags);

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

void ComputePass::CreatePipelineState()
{
	m_ComputeShader = std::make_unique<Shader>(StringHelper::StringToWString(m_Desc.ComputeShaderPath), "main", "cs_6_0");

	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.CS = m_ComputeShader->GetShaderByteCode();
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
	psoDesc.NodeMask = 0;
	psoDesc.pRootSignature = m_d3d12RootSignature.Get();

	DX_CALL(RenderBackend::GetD3D12Device()->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&m_d3d12PipelineState)));
	m_d3d12PipelineState->SetName(StringHelper::StringToWString(m_Name + " pipeline state").c_str());
}
