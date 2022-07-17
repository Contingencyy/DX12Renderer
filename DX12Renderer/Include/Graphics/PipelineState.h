#pragma once
#include "Graphics/Texture.h"

class RootSignature;
class Shader;

struct PipelineStateDesc
{
	PipelineStateDesc() = default;
	PipelineStateDesc(const std::wstring& vsPath, const std::wstring& psPath, const TextureDesc& colorAttachDesc, const TextureDesc& depthAttachDesc, bool depthTest)
		: VertexShaderPath(vsPath), PixelShaderPath(psPath), ColorAttachmentDesc(colorAttachDesc), DepthStencilAttachmentDesc(depthAttachDesc), DepthTest(depthTest) {}

	std::wstring VertexShaderPath = L"Resources/Shaders/Default_VS.hlsl";
	std::wstring PixelShaderPath = L"Resources/Shaders/Default_PS.hlsl";

	TextureDesc ColorAttachmentDesc = TextureDesc();
	TextureDesc DepthStencilAttachmentDesc = TextureDesc();

	bool DepthTest = true;
};

class PipelineState
{
public:
	PipelineState(const PipelineStateDesc& pipelineStateDesc, const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout,
		const std::vector<CD3DX12_DESCRIPTOR_RANGE1>& descriptorRanges, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters);
	~PipelineState();

	RootSignature* GetRootSignature() const { return m_RootSignature.get(); }
	ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const { return m_d3d12PipelineState; }

	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_d3d12PrimitiveToplogy; }

	Texture& GetColorAttachment() { return *m_ColorAttachment.get(); }
	const Texture& GetColorAttachment() const { return *m_ColorAttachment.get(); }
	Texture& GetDepthStencilAttachment() { return *m_DepthStencilAttachment.get(); }
	const Texture& GetDepthStencilAttachment() const { return *m_DepthStencilAttachment.get(); }
	
private:
	void Create(const std::vector<D3D12_INPUT_ELEMENT_DESC>& inputLayout);

private:
	PipelineStateDesc m_PipelineStateDesc;

	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY m_d3d12PrimitiveToplogy;

	std::unique_ptr<Shader> m_VertexShader;
	std::unique_ptr<Shader> m_PixelShader;
	std::unique_ptr<RootSignature> m_RootSignature;

	std::unique_ptr<Texture> m_ColorAttachment;
	std::unique_ptr<Texture> m_DepthStencilAttachment;

};
