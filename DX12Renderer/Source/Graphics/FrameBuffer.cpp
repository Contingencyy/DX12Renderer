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
	for (auto& colorAttachment : m_ColorAttachments)
		colorAttachment->Resize(width, height);

	for (auto& depthAttachment : m_DepthAttachments)
		depthAttachment->Resize(width, height);
}

void FrameBuffer::ClearAttachments()
{
	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

	for (auto& colorAttachment : m_ColorAttachments)
		if (colorAttachment->IsValid())
			commandList->ClearRenderTargetView(colorAttachment->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(colorAttachment->GetTextureDesc().ClearColor));

	for (auto& depthAttachment : m_DepthAttachments)
		if (depthAttachment->IsValid())
			commandList->ClearDepthStencilView(depthAttachment->GetDescriptor(DescriptorType::DSV), depthAttachment->GetTextureDesc().ClearDepthStencil.x);

	RenderBackend::ExecuteCommandList(commandList);
}

void FrameBuffer::Invalidate()
{
	ReleaseTextures();
	MakeTextures();
}

Texture& FrameBuffer::GetColorAttachment()
{
	return *m_ColorAttachments[m_CurrentFrameIndex];
}

const Texture& FrameBuffer::GetColorAttachment() const
{
	return *m_ColorAttachments[m_CurrentFrameIndex];
}

Texture& FrameBuffer::GetDepthAttachment()
{
	return *m_DepthAttachments[m_CurrentFrameIndex];
}

const Texture& FrameBuffer::GetDepthAttachment() const
{
	return *m_DepthAttachments[m_CurrentFrameIndex];
}

void FrameBuffer::MakeTextures()
{
	for (auto& colorAttachment : m_ColorAttachments)
		colorAttachment = std::make_unique<Texture>(m_Name + " color attachment", m_FrameBufferDesc.ColorAttachmentDesc);

	for (auto& depthAttachment : m_DepthAttachments)
		depthAttachment = std::make_unique<Texture>(m_Name + " depth attachment", m_FrameBufferDesc.DepthAttachmentDesc);
}

void FrameBuffer::ReleaseTextures()
{
	for (auto& colorAttachment : m_ColorAttachments)
		colorAttachment.release();

	for (auto& depthAttachment : m_DepthAttachments)
		depthAttachment.release();
}
