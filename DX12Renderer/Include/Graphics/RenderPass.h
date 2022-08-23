#pragma once
#include "Graphics/Texture.h"

class PipelineState;
class Texture;
class CommandList;

struct RenderPassDesc
{
	std::string VertexShaderPath;
	std::string PixelShaderPath;

	TextureDesc ColorAttachmentDesc;
	TextureDesc DepthAttachmentDesc;
	
	glm::vec4 ClearColor;
	bool DepthEnabled;

	D3D12_PRIMITIVE_TOPOLOGY Topology;
	D3D12_PRIMITIVE_TOPOLOGY_TYPE TopologyType;

	std::vector<CD3DX12_DESCRIPTOR_RANGE1> DescriptorRanges;
	std::vector<CD3DX12_ROOT_PARAMETER1> RootParameters;
	std::vector<D3D12_INPUT_ELEMENT_DESC> ShaderInputLayout;
};

class RenderPass
{
public:
	RenderPass(const RenderPassDesc& desc);
	~RenderPass();

	void Resize(uint32_t width, uint32_t height);
	void ClearViews(const std::shared_ptr<CommandList>& commandList);

	Texture& GetColorAttachment() { return *m_ColorAttachment; }
	const Texture& GetColorAttachment() const { return *m_ColorAttachment; }
	Texture& GetDepthAttachment() { return *m_DepthAttachment; }
	const Texture& GetDepthAttachment() const { return *m_DepthAttachment; }

	// Temp
	const PipelineState& GetPipelineState() const { return *m_PipelineState; }

private:
	RenderPassDesc m_Desc;

	std::unique_ptr<PipelineState> m_PipelineState;

	std::unique_ptr<Texture> m_ColorAttachment;
	std::unique_ptr<Texture> m_DepthAttachment;

};
