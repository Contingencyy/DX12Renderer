#pragma once

class Renderer
{
public:
	Renderer();
	~Renderer();

	void Initialize(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight);
	void Render();
	void Finalize();

	void Resize(uint32_t width, uint32_t height);

private:
	void Present();
	void Flush();

	void EnableDebugLayer();

	void CreateAdapter();
	void CreateDevice();

	void CreateDescriptorHeap();
	void CreateCommandQueue();
	void CreateSwapChain(HWND hWnd);

	void CreateRootSignature();
	void CreatePipelineState();

	void UpdateRenderTargetViews();

private:
	static const uint32_t s_BackBufferCount = 3;

	ComPtr<IDXGIAdapter4> m_dxgiAdapter;
	ComPtr<ID3D12Device2> m_d3d12Device;

	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap;
	uint32_t m_RTVDescriptorSize;

	ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
	ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
	ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocators[s_BackBufferCount];

	ComPtr<ID3D12Fence> m_d3d12Fence;
	uint64_t m_FenceValue = 0;
	uint64_t m_FenceValues[s_BackBufferCount] = {};
	HANDLE m_FenceEvent;

	ComPtr<IDXGISwapChain4> m_dxgiSwapChain;
	ComPtr<ID3D12Resource> m_BackBuffers[s_BackBufferCount];
	uint32_t m_CurrentBackBufferIndex = 0;

	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;
	ComPtr<ID3D12PipelineState> m_d3d12PipelineState;
	
	bool m_VSync = true;
	bool m_TearingSupported = false;

	struct Resolution
	{
		uint32_t x;
		uint32_t y;
	};

	Resolution m_Resolution;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

};
