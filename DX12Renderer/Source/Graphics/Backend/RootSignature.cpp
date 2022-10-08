#include "Pch.h"
#include "Graphics/Backend/RootSignature.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/RenderBackend.h"

RootSignature::RootSignature(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters)
{
	Create(name, rootParameters);
}

RootSignature::~RootSignature()
{
}

void RootSignature::Create(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters)
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
	staticSamplers[0].MaxAnisotropy = 16;
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
	staticSamplers[1].Filter = D3D12_FILTER_MAXIMUM_MIN_MAG_MIP_POINT;
	staticSamplers[1].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	staticSamplers[1].MaxAnisotropy = 1;
	staticSamplers[1].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[1].BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
	staticSamplers[1].MinLOD = 0.0f;
	staticSamplers[1].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[1].MipLODBias = 0;
	staticSamplers[1].ShaderRegister = 0;
	staticSamplers[1].RegisterSpace = 1;
	staticSamplers[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(rootParameters.size(), &rootParameters[0], _countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);

	RenderBackend::GetDevice()->CreateRootSignature(versionedRootSignatureDesc, m_d3d12RootSignature);
	m_d3d12RootSignature->SetName(StringHelper::StringToWString(name + " root signature").c_str());
}
