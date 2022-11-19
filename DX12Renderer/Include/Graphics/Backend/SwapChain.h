#pragma once

class CommandQueue;
class Texture;

class SwapChain
{
public:
	SwapChain(HWND hWnd, std::shared_ptr<CommandQueue> commandQueue, uint32_t width, uint32_t height);
	~SwapChain();

	void ResolveToBackBuffer(Texture& texture);
	void SwapBuffers(bool vSync);
	void Resize(uint32_t width, uint32_t height);

	uint32_t GetBackBufferCount() const { return s_BackBufferCount; }
	uint32_t GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

private:
	void CreateBackBufferTextures();
	void ResizeBackBuffers(uint32_t width, uint32_t height);

private:
	static const uint32_t s_BackBufferCount = 3;

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;

	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::unique_ptr<Texture> m_BackBuffers[s_BackBufferCount];

	uint32_t m_CurrentBackBufferIndex = 0;
	uint64_t m_FenceValues[s_BackBufferCount] = {};

	bool m_TearingSupported = false;

};
