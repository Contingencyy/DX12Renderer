#include "Pch.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/RenderBackend.h"

SwapChain::SwapChain(HWND hWnd, std::shared_ptr<CommandQueue> commandQueue, uint32_t width, uint32_t height)
    : CommandQueueDirect(commandQueue)
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
    DX_CALL(dxgiFactory5->CreateSwapChainForHwnd(CommandQueueDirect->GetD3D12CommandQueue().Get(),
        hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
    DX_CALL(dxgiSwapChain1.As(&m_dxgiSwapChain));

    DX_CALL(dxgiFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();

    CreateBackBufferTextures();
}

SwapChain::~SwapChain()
{
}

void SwapChain::ResolveToBackBuffer(const Texture& texture)
{
    SCOPED_TIMER("SwapChain::ResolveToBackBuffer");

    auto commandList = CommandQueueDirect->GetCommandList();
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    CD3DX12_RESOURCE_BARRIER copyBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(backBuffer->GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_COPY_DEST),
        CD3DX12_RESOURCE_BARRIER::Transition(texture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COPY_SOURCE)
    };
    commandList->ResourceBarrier(2, copyBarriers);

    commandList->ResolveTexture(texture, *backBuffer);

    CD3DX12_RESOURCE_BARRIER renderBarriers[] = {
        CD3DX12_RESOURCE_BARRIER::Transition(backBuffer->GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PRESENT),
        CD3DX12_RESOURCE_BARRIER::Transition(texture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET)
    };
    commandList->ResourceBarrier(2, renderBarriers);

    CommandQueueDirect->ExecuteCommandList(commandList);
}

void SwapChain::SwapBuffers(bool vSync)
{
    SCOPED_TIMER("SwapChain::SwapBuffers");

    unsigned int syncInterval = vSync ? 1 : 0;
    unsigned int presentFlags = m_TearingSupported && !vSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    DX_CALL(m_dxgiSwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = CommandQueueDirect->Signal();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    CommandQueueDirect->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);
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
            TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM, backBufferDesc.Width, backBufferDesc.Height));
        m_BackBuffers[i]->SetD3D12Resource(backBuffer);
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