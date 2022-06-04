#include "Pch.h"
#include "Graphics/SwapChain.h"
#include "Graphics/Texture.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"

SwapChain::SwapChain(HWND hWnd, uint32_t width, uint32_t height)
{
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGIFactory5> dxgiFactory5;

    DX_CALL(Application::Get().GetRenderer()->GetDevice()->GetDXGIAdapter()->GetParent(IID_PPV_ARGS(&dxgiFactory)));
    DX_CALL(dxgiFactory.As(&dxgiFactory5));

    BOOL allowTearing = false;
    if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL))))
    {
        m_TearingSupported = (allowTearing == TRUE);
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = width;
    swapChainDesc.Height = height;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = s_BackBufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    auto commandQueueDirect = Application::Get().GetRenderer()->GetCommandQueueDirect();

    ComPtr<IDXGISwapChain1> dxgiSwapChain1;
    DX_CALL(dxgiFactory5->CreateSwapChainForHwnd(commandQueueDirect->GetD3D12CommandQueue().Get(),
        hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
    DX_CALL(dxgiSwapChain1.As(&m_dxgiSwapChain));

    DX_CALL(dxgiFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    CreateDepthBufferTexture(width, height);
    CreateBackBufferTextures();
}

SwapChain::~SwapChain()
{
}

void SwapChain::SwapBuffers()
{
    auto commandQueueDirect = Application::Get().GetRenderer()->GetCommandQueueDirect();
    auto commandList = commandQueueDirect->GetCommandList();
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer->GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);
    commandQueueDirect->ExecuteCommandList(commandList);

    Renderer::RenderSettings renderSettings = Application::Get().GetRenderer()->GetRenderSettings();

    unsigned int syncInterval = renderSettings.VSync ? 1 : 0;
    unsigned int presentFlags = m_TearingSupported && !renderSettings.VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    DX_CALL(m_dxgiSwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = commandQueueDirect->Signal();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    commandQueueDirect->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    ResizeBackBuffers(width, height);
    m_DepthBuffer->Resize(width, height);

    CreateBackBufferTextures();
}

void SwapChain::CreateBackBufferTextures()
{
    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        DX_CALL(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        D3D12_RESOURCE_DESC backBufferDesc = backBuffer->GetDesc();

        m_BackBuffers[i] = std::make_shared<Texture>(TextureDesc(backBufferDesc.Format, D3D12_RESOURCE_STATE_RENDER_TARGET,
            D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET, backBufferDesc.Width, backBufferDesc.Height));
        m_BackBuffers[i]->SetD3D12Resource(backBuffer);
    }
}


void SwapChain::CreateDepthBufferTexture(uint32_t width, uint32_t height)
{
    m_DepthBuffer = std::make_shared<Texture>(TextureDesc(DXGI_FORMAT_D32_FLOAT, D3D12_RESOURCE_STATE_DEPTH_WRITE,
        D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL, width, height));
}

void SwapChain::ResizeBackBuffers(uint32_t width, uint32_t height)
{
    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        m_BackBuffers[i]->GetD3D12Resource().Reset();
        m_BackBuffers[i].reset();

        m_FenceValues[i] = m_FenceValues[m_CurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    DX_CALL(m_dxgiSwapChain->GetDesc(&swapChainDesc));
    DX_CALL(m_dxgiSwapChain->ResizeBuffers(s_BackBufferCount, width, height,
        swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}