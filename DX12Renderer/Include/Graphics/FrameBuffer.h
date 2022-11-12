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

	void SetFrameIndex(uint32_t frameIndex) { m_CurrentFrameIndex = frameIndex; }

private:
	void MakeTextures();
	void ReleaseTextures();

private:
	FrameBufferDesc m_FrameBufferDesc;
	std::array<std::unique_ptr<Texture>, 3> m_ColorAttachments;
	std::array<std::unique_ptr<Texture>, 3> m_DepthAttachments;

	std::string m_Name = "";
	uint32_t m_CurrentFrameIndex = 0;

};
