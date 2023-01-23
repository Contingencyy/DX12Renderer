#pragma once
#include "Graphics/RenderState.h"

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

	uint32_t GetCurrentBackBufferIndex() const { return m_CurrentBackBufferIndex; }

private:
	void CreateBackBufferTextures();
	void ResizeBackBuffers(uint32_t width, uint32_t height);

private:
	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;

	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::unique_ptr<Texture> m_BackBuffers[RenderState::BACK_BUFFER_COUNT];

	uint32_t m_CurrentBackBufferIndex = 0;
	uint64_t m_FenceValues[RenderState::BACK_BUFFER_COUNT] = {};

	bool m_TearingSupported = false;

};
