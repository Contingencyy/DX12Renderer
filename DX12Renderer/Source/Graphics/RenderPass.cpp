#include "Pch.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/PipelineState.h"
#include "Graphics/Backend/CommandList.h"

RenderPass::RenderPass(const std::string& name, const RenderPassDesc& desc)
	: m_Name(name), m_Desc(desc)
{
	m_PipelineState = std::make_unique<PipelineState>(name, m_Desc);

	m_ColorAttachment = std::make_unique<Texture>(name + " color frame buffer", m_Desc.ColorAttachmentDesc);
	m_DepthAttachment = std::make_unique<Texture>(name + " depth stencil frame buffer", m_Desc.DepthAttachmentDesc);
}

RenderPass::~RenderPass()
{
}

void RenderPass::Resize(uint32_t width, uint32_t height)
{
	m_ColorAttachment->Resize(width, height);
	m_DepthAttachment->Resize(width, height);
}

void RenderPass::ClearViews(const std::shared_ptr<CommandList>& commandList)
{
	if (m_ColorAttachment->IsValid())
		commandList->ClearRenderTargetView(m_ColorAttachment->GetDescriptorHandle(DescriptorType::RTV), glm::value_ptr<float>(m_ColorAttachment->GetTextureDesc().ClearColor));
	if (m_DepthAttachment->IsValid())
		commandList->ClearDepthStencilView(m_DepthAttachment->GetDescriptorHandle(DescriptorType::DSV));
}
