#pragma once

class CommandQueue;
class Texture;

class SwapChain
{
public:
	SwapChain(HWND hWnd, uint32_t width, uint32_t height);
	~SwapChain();

	void SwapBuffers();
	void Resize(uint32_t width, uint32_t height);

	std::shared_ptr<Texture> GetColorTarget() const { return m_ColorTargetTextures[m_CurrentBackBufferIndex]; }
	std::shared_ptr<Texture> GetDepthBuffer() const { return m_DepthBuffer; }
	uint32_t GetBackBufferCount() const { return s_BackBufferCount; }

private:
	void ResolveBackBuffer();

	void CreateBackBufferTextures();
	void CreateRenderTargetTextures();
	void CreateDepthBufferTexture(uint32_t width, uint32_t height);
	void ResizeBackBuffers(uint32_t width, uint32_t height);

private:
	static const uint32_t s_BackBufferCount = 3;

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;

	std::unique_ptr<Texture> m_BackBuffers[s_BackBufferCount];
	std::shared_ptr<Texture> m_ColorTargetTextures[s_BackBufferCount];
	std::shared_ptr<Texture> m_DepthBuffer;

	uint32_t m_CurrentBackBufferIndex = 0;
	uint64_t m_FenceValues[s_BackBufferCount] = {};

	bool m_TearingSupported = false;

};