#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"

class CommandQueue;

class Renderer
{
public:
	struct RenderSettings
	{
		struct Resolution
		{
			uint32_t x;
			uint32_t y;
		} Resolution;

		bool VSync = true;
		bool TearingSupported = false;
	};

public:
	Renderer();
	~Renderer();

	void Initialize(uint32_t width, uint32_t height);
	void BeginFrame();
	void Render();
	void EndFrame();
	void Finalize();

	void Resize(uint32_t width, uint32_t height);

	void CreateBuffer(ComPtr<ID3D12Resource>& resource, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size);
	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CreateTexture(ComPtr<ID3D12Resource>& resource, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, std::size_t size);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);

	void ToggleVSync() { m_RenderSettings.VSync = !m_RenderSettings.VSync; }
	bool IsVSyncEnabled() const { return m_RenderSettings.VSync; }

	RenderSettings GetRenderSettings() const { return m_RenderSettings; }
	ComPtr<ID3D12Device> GetD3D12Device() const { return m_d3d12Device; }

private:
	void Present();
	void Flush();

	void EnableDebugLayer();

	void CreateAdapter();
	void CreateDevice();

	void CreateDescriptorHeap();
	void CreateSwapChain(HWND hWnd);
	void CreateDepthBuffer();

	void CreateRootSignature();
	void CreatePipelineState();

	void ResizeBackBuffers();
	void UpdateRenderTargetViews();

private:
	friend class GUI;

	static const uint32_t s_BackBufferCount = 3;

	ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	ComPtr<ID3D12Device2> m_d3d12Device;

	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorOffsets[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::shared_ptr<CommandQueue> m_CommandQueueCopy;
	uint64_t m_FenceValues[s_BackBufferCount] = {};

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	ComPtr<ID3D12Resource> m_BackBuffers[s_BackBufferCount];
	ComPtr<ID3D12Resource> m_DepthBuffer;
	uint32_t m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;

	std::shared_ptr<Buffer> m_QuadBuffers[3];
	std::shared_ptr<Texture> m_Texture;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	RenderSettings m_RenderSettings;

};
