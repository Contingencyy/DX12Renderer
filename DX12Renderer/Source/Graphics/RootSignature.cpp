#include "Pch.h"
#include "Graphics/RootSignature.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

RootSignature::RootSignature(const std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters)
{
	Create(descriptorRanges, rootParameters);
}

RootSignature::~RootSignature()
{
}

void RootSignature::Create(const std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters)
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MipLODBias = 0;
	staticSamplers[0].MaxAnisotropy = 0;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(rootParameters.size(), &rootParameters[0], _countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);

	Application::Get().GetRenderer()->GetDevice()->CreateRootSignature(versionedRootSignatureDesc, m_d3d12RootSignature);
	
	ParseDescriptorTableRanges(versionedRootSignatureDesc.Desc_1_1);
}

void RootSignature::ParseDescriptorTableRanges(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc)
{
	uint32_t numRootParameters = rootSignatureDesc.NumParameters;

	if (numRootParameters > 0)
	{
		D3D12_ROOT_PARAMETER1* rootParameterPtr = new D3D12_ROOT_PARAMETER1[numRootParameters];

		for (uint32_t i = 0; i < numRootParameters; ++i)
		{
			const D3D12_ROOT_PARAMETER1& rootParameter = rootSignatureDesc.pParameters[i];
			rootParameterPtr[i] = rootParameter;

			if (rootParameter.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
			{
				uint32_t numDescriptorRanges = rootParameter.DescriptorTable.NumDescriptorRanges;

				if (numDescriptorRanges > 0)
				{
					D3D12_DESCRIPTOR_RANGE1* descriptorRangesPtr = new D3D12_DESCRIPTOR_RANGE1[numDescriptorRanges];
					memcpy(descriptorRangesPtr, rootParameter.DescriptorTable.pDescriptorRanges, sizeof(D3D12_DESCRIPTOR_RANGE1) * numDescriptorRanges);

					rootParameterPtr[i].DescriptorTable.NumDescriptorRanges = numDescriptorRanges;
					rootParameterPtr[i].DescriptorTable.pDescriptorRanges = descriptorRangesPtr;

					for (uint32_t j = 0; j < numDescriptorRanges; ++j)
					{
						switch (descriptorRangesPtr[0].RangeType)
						{
						case D3D12_DESCRIPTOR_RANGE_TYPE_CBV:
						case D3D12_DESCRIPTOR_RANGE_TYPE_SRV:
						case D3D12_DESCRIPTOR_RANGE_TYPE_UAV:
							m_DescriptorTableRanges.emplace_back(i, descriptorRangesPtr[j].OffsetInDescriptorsFromTableStart,
								descriptorRangesPtr[j].NumDescriptors);
							break;
						case D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER:
							m_SamplerTableRanges.emplace_back(i, descriptorRangesPtr[j].OffsetInDescriptorsFromTableStart,
								descriptorRangesPtr[j].NumDescriptors);
							break;
						}
					}
				}
			}
		}
	}
}
