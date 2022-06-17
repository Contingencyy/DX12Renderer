#pragma once
#include "Graphics/Texture.h"

class RootSignature;
class Shader;

struct PipelineStateDesc
{
	std::wstring VertexShaderPath;
	std::wstring PixelShaderPath;

	TextureDesc ColorAttachmentDesc;
	TextureDesc DepthStencilAttachmentDesc;
};

class PipelineState
{
public:
	PipelineState(const PipelineStateDesc& pipelineStateDesc);
	~PipelineState();

	RootSignature* GetRootSignature() const { return m_RootSignature.get(); }
	ComPtr<ID3D12PipelineState> GetD3D12PipelineState() const { return m_d3d12PipelineState; }

	D3D12_PRIMITIVE_TOPOLOGY GetPrimitiveTopology() const { return m_d3d12PrimitiveToplogy; }

	Texture& GetColorAttachment() { return *m_ColorAttachment.get(); }
	const Texture& GetColorAttachment() const { return *m_ColorAttachment.get(); }
	Texture& GetDepthStencilAttachment() { return *m_DepthStencilAttachment.get(); }
	const Texture& GetDepthStencilAttachment() const { return *m_DepthStencilAttachment.get(); }
	
private:
	void Create();

private:
	PipelineStateDesc m_PipelineStateDesc;
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	D3D12_PRIMITIVE_TOPOLOGY m_d3d12PrimitiveToplogy;

	std::unique_ptr<RootSignature> m_RootSignature;

	std::unique_ptr<Texture> m_ColorAttachment;
	std::unique_ptr<Texture> m_DepthStencilAttachment;

};
