#include "Pch.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/PipelineState.h"
#include "Graphics/Backend/CommandList.h"

RenderPass::RenderPass(const std::string& name, const RenderPassDesc& desc)
	: m_Name(name), m_RenderPassDesc(desc)
{
	m_PipelineState = std::make_unique<PipelineState>(name, m_RenderPassDesc);
}

RenderPass::~RenderPass()
{
}
