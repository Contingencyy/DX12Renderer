#include "Pch.h"
#include "Graphics/RenderPass.h"
#include "Graphics/PipelineState.h"
#include "Graphics/CommandList.h"

RenderPass::RenderPass(const RenderPassDesc& desc)
	: m_Desc(desc)
{
	m_PipelineState = std::make_unique<PipelineState>(m_Desc);

	m_ColorAttachment = std::make_unique<Texture>(m_Desc.ColorAttachmentDesc);
	m_DepthAttachment = std::make_unique<Texture>(m_Desc.DepthAttachmentDesc);
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
	commandList->ClearRenderTargetView(m_ColorAttachment->GetRenderTargetDepthStencilView(), &m_Desc.ClearColor.x);
	commandList->ClearDepthStencilView(m_DepthAttachment->GetRenderTargetDepthStencilView());
}
