#include "Pch.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/RenderBackend.h"

SwapChain::SwapChain(HWND hWnd, std::shared_ptr<CommandQueue> commandQueue, uint32_t width, uint32_t height)
    : m_CommandQueueDirect(commandQueue)
{
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGIFactory5> dxgiFactory5;

    DX_CALL(RenderBackend::GetDevice()->GetDXGIAdapter()->GetParent(IID_PPV_ARGS(&dxgiFactory)));
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

    ComPtr<IDXGISwapChain1> dxgiSwapChain1;
    DX_CALL(dxgiFactory5->CreateSwapChainForHwnd(m_CommandQueueDirect->GetD3D12CommandQueue().Get(),
        hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
    DX_CALL(dxgiSwapChain1.As(&m_dxgiSwapChain));

    DX_CALL(dxgiFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    CreateBackBufferTextures();
}

SwapChain::~SwapChain()
{
}

void SwapChain::ResolveToBackBuffer(Texture& texture)
{
    SCOPED_TIMER("SwapChain::ResolveToBackBuffer");

    auto commandList = m_CommandQueueDirect->GetCommandList();
    commandList->ResolveTexture(texture, *m_BackBuffers[m_CurrentBackBufferIndex]);
    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void SwapChain::SwapBuffers(bool vSync)
{
    SCOPED_TIMER("SwapChain::SwapBuffers");

    auto commandList = m_CommandQueueDirect->GetCommandList();
    commandList->Transition(*m_BackBuffers[m_CurrentBackBufferIndex], D3D12_RESOURCE_STATE_PRESENT);
    m_CommandQueueDirect->ExecuteCommandList(commandList);

    unsigned int syncInterval = vSync ? 1 : 0;
    unsigned int presentFlags = m_TearingSupported && !vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    DX_CALL(m_dxgiSwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = m_CommandQueueDirect->Signal();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    m_CommandQueueDirect->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);
}

void SwapChain::Resize(uint32_t width, uint32_t height)
{
    ResizeBackBuffers(width, height);
    CreateBackBufferTextures();
}

void SwapChain::CreateBackBufferTextures()
{
    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        DX_CALL(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        D3D12_RESOURCE_DESC backBufferDesc = backBuffer->GetDesc();

        m_BackBuffers[i] = std::make_unique<Texture>("Back buffer", TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET,
            TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM, static_cast<uint32_t>(backBufferDesc.Width), static_cast<uint32_t>(backBufferDesc.Height)));
        m_BackBuffers[i]->SetD3D12Resource(backBuffer);
        m_BackBuffers[i]->SetD3D12Resourcestate(D3D12_RESOURCE_STATE_PRESENT);
    }
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