#pragma once
#include "Texture.h"

struct FrameBufferDesc
{
	TextureDesc ColorAttachmentDesc;
	TextureDesc DepthAttachmentDesc;
};

class FrameBuffer
{
public:
	FrameBuffer(const std::string& name, const FrameBufferDesc& frameBufferDesc);
	~FrameBuffer();

	void Resize(uint32_t width, uint32_t height);
	void ClearAttachments();
	void Invalidate();

	FrameBufferDesc& GetFrameBufferDesc() { return m_FrameBufferDesc; }
	const FrameBufferDesc& GetFrameBufferDesc() const { return m_FrameBufferDesc; }
	Texture& GetColorAttachment();
	const Texture& GetColorAttachment() const;
	Texture& GetDepthAttachment();
	const Texture& GetDepthAttachment() const;

private:
	void MakeTextures();
	void ReleaseTextures();

private:
	FrameBufferDesc m_FrameBufferDesc;
	std::unique_ptr<Texture> m_ColorAttachment;
	std::unique_ptr<Texture> m_DepthAttachment;

	std::string m_Name = "";
	uint32_t m_CurrentFrameIndex = 0;

};
