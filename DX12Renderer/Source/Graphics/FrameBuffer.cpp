#include "Pch.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/CommandList.h"

FrameBuffer::FrameBuffer(const std::string& name, const FrameBufferDesc& frameBufferDesc)
	: m_Name(name), m_FrameBufferDesc(frameBufferDesc)
{
	MakeTextures();
}

FrameBuffer::~FrameBuffer()
{
}

void FrameBuffer::Resize(uint32_t width, uint32_t height)
{
	m_ColorAttachment->Resize(width, height);
	m_DepthAttachment->Resize(width, height);
}

void FrameBuffer::ClearAttachments()
{
	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

	if (m_ColorAttachment->IsValid())
	{
		commandList->Transition(*m_ColorAttachment, D3D12_RESOURCE_STATE_RENDER_TARGET);
		commandList->ClearRenderTargetView(m_ColorAttachment->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(m_ColorAttachment->GetTextureDesc().ClearColor));
	}

	if (m_DepthAttachment->IsValid())
	{
		commandList->Transition(*m_DepthAttachment, D3D12_RESOURCE_STATE_DEPTH_WRITE);
		commandList->ClearDepthStencilView(m_DepthAttachment->GetDescriptor(DescriptorType::DSV), m_DepthAttachment->GetTextureDesc().ClearDepthStencil.x);
	}

	RenderBackend::ExecuteCommandList(commandList);
}

void FrameBuffer::Invalidate()
{
	ReleaseTextures();
	MakeTextures();
}

Texture& FrameBuffer::GetColorAttachment()
{
	return *m_ColorAttachment;
}

const Texture& FrameBuffer::GetColorAttachment() const
{
	return *m_ColorAttachment;
}

Texture& FrameBuffer::GetDepthAttachment()
{
	return *m_DepthAttachment;
}

const Texture& FrameBuffer::GetDepthAttachment() const
{
	return *m_DepthAttachment;
}

void FrameBuffer::MakeTextures()
{
	m_ColorAttachment = std::make_unique<Texture>(m_Name + " color attachment", m_FrameBufferDesc.ColorAttachmentDesc);
	m_DepthAttachment = std::make_unique<Texture>(m_Name + " depth attachment", m_FrameBufferDesc.DepthAttachmentDesc);
}

void FrameBuffer::ReleaseTextures()
{
	m_ColorAttachment.release();
	m_DepthAttachment.release();
}
