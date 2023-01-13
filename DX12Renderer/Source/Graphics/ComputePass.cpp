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

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(static_cast<uint32_t>(m_Desc.RootParameters.size()),
		&m_Desc.RootParameters[0], 0, nullptr, rootSignatureFlags);

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
