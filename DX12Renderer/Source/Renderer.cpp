#include "Pch.h"
#include "Renderer.h"
#include "Graphics/VertexBuffer.h"
#include "Graphics/IndexBuffer.h"
#include "Graphics/UploadBuffer.h"

std::vector<glm::vec3> QuadVertices = {
    glm::vec3(-0.5f, -0.5f, 0.0f),
    glm::vec3(-0.5f, 0.5f, 0.0f),
    glm::vec3(0.5f, 0.5f, 0.0f),
    glm::vec3(0.5f, -0.5f, 0.0f)
};

struct InstanceData
{
    glm::mat4 ModelMatrix;
    glm::vec3 Color;
};

std::vector<InstanceData> QuadInstanceData = {
    InstanceData{ glm::identity<glm::mat4>(), glm::vec3(0.8f, 0.0f, 0.8f) }
};

std::vector<WORD> QuadIndices = {
    0, 1, 2,
    2, 3, 0
};

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Initialize(HWND hWnd, uint32_t windowWidth, uint32_t windowHeight)
{
	m_Resolution.x = windowWidth;
	m_Resolution.y = windowHeight;

	EnableDebugLayer();

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Resolution.x), static_cast<float>(m_Resolution.y), 0.0f, 1.0f);
	m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    CreateAdapter();
    CreateDevice();

    CreateDescriptorHeap();
    CreateCommandQueue();
    CreateSwapChain(hWnd);
    CreateDepthBuffer();

    CreateRootSignature();
    CreatePipelineState();

    UpdateRenderTargetViews();

    QuadInstanceData.clear();
    QuadInstanceData.reserve(100);

    for (uint32_t i = 0; i < 100; ++i)
    {
        QuadInstanceData.emplace_back(InstanceData{ glm::scale(glm::translate(glm::identity<glm::mat4>(),
            glm::vec3(Random::FloatRange(100.0f, -100.0f), Random::FloatRange(100.0f, -100.0f), 0.0f)),
            glm::vec3(Random::FloatRange(5.0f, 15.0f))), glm::vec3(Random::Float(), Random::Float(), Random::Float()) });
    }
}

void Renderer::Render()
{
    auto& commandAllocator = m_d3d12CommandAllocators[m_CurrentBackBufferIndex];
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    commandAllocator->Reset();
    m_d3d12CommandList->Reset(commandAllocator.Get(), nullptr);
    ReleaseTrackedResources();

    // Transition back buffer to render target state
    {
        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
            D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        m_d3d12CommandList->ResourceBarrier(1, &barrier);
    }

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
        m_CurrentBackBufferIndex, m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());

    // Clear render target and depth stencil view
    {
        float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
        m_d3d12CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
        m_d3d12CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0, 0, nullptr);
    }

    // Render quads
    {
        m_d3d12CommandList->RSSetViewports(1, &m_Viewport);
        m_d3d12CommandList->RSSetScissorRects(1, &m_ScissorRect);
        m_d3d12CommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

        m_d3d12CommandList->SetPipelineState(m_d3d12PipelineState.Get());
        m_d3d12CommandList->SetGraphicsRootSignature(m_d3d12RootSignature.Get());
        m_d3d12CommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        glm::mat4 projection = glm::perspectiveFovLH_ZO(glm::radians(60.0f), static_cast<float>(m_Resolution.x), static_cast<float>(m_Resolution.y), 0.1f, 1000.0f);
        glm::mat4 view = glm::lookAtLH(glm::vec3(0.0f, 0.0f, -250.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 viewProjection = projection * view;
        m_d3d12CommandList->SetGraphicsRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);

        // New vertex/index/upload buffer classes
        UploadBuffer quadVerticesUploadBuffer(QuadVertices.size() * sizeof(glm::vec3));
        VertexBuffer quadVertexBuffer;
        quadVerticesUploadBuffer.UploadDataToBuffer(quadVertexBuffer, QuadVertices.size(),
            sizeof(glm::vec3), &QuadVertices[0]);

        TrackResource(quadVerticesUploadBuffer.GetD3D12Resource());
        TrackResource(quadVertexBuffer.GetD3D12Resource());

        UploadBuffer quadInstanceDataUploadBuffer(QuadInstanceData.size() * sizeof(InstanceData));
        VertexBuffer instanceDataBuffer;
        quadInstanceDataUploadBuffer.UploadDataToBuffer(instanceDataBuffer, QuadInstanceData.size(),
            sizeof(InstanceData), &QuadInstanceData[0]);

        TrackResource(quadInstanceDataUploadBuffer.GetD3D12Resource());
        TrackResource(instanceDataBuffer.GetD3D12Resource());

        UploadBuffer quadIndicesUploadBuffer(QuadIndices.size() * sizeof(WORD));
        IndexBuffer quadIndexBuffer;
        quadIndicesUploadBuffer.UploadDataToBuffer(quadIndexBuffer, QuadIndices.size(), sizeof(WORD), &QuadIndices[0]);

        TrackResource(quadIndicesUploadBuffer.GetD3D12Resource());
        TrackResource(quadIndexBuffer.GetD3D12Resource());

        D3D12_VERTEX_BUFFER_VIEW quadVertexBufferView = quadVertexBuffer.GetView();
        D3D12_VERTEX_BUFFER_VIEW instanceDataBufferView = instanceDataBuffer.GetView();
        D3D12_INDEX_BUFFER_VIEW quadIndexBufferView = quadIndexBuffer.GetView();

        m_d3d12CommandList->IASetVertexBuffers(0, 1, &quadVertexBufferView);
        m_d3d12CommandList->IASetVertexBuffers(1, 1, &instanceDataBufferView);
        m_d3d12CommandList->IASetIndexBuffer(&quadIndexBufferView);

        m_d3d12CommandList->DrawIndexedInstanced(QuadIndices.size(), QuadInstanceData.size(), 0, 0, 0);
    }

    Present();
}

void Renderer::Finalize()
{
    Flush();
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (m_Resolution.x != width || m_Resolution.y != height)
    {
        m_Resolution.x = std::max(1u, width);
        m_Resolution.y = std::max(1u, height);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_Resolution.x), static_cast<float>(m_Resolution.y), 0.0f, 1.0f);

        Flush();

        ResizeBackBuffers();
        CreateDepthBuffer();

        UpdateRenderTargetViews();
    }
}

void Renderer::Present()
{
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    m_d3d12CommandList->ResourceBarrier(1, &barrier);

    DX_CALL(m_d3d12CommandList->Close());

    ID3D12CommandList* const commandLists[] = {
        m_d3d12CommandList.Get()
    };
    m_d3d12CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

    unsigned int syncInterval = m_VSync ? 1 : 0;
    unsigned int presentFlags = m_TearingSupported && !m_VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    DX_CALL(m_dxgiSwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = ++m_FenceValue;
    DX_CALL(m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_FenceValues[m_CurrentBackBufferIndex]));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    if (m_d3d12Fence->GetCompletedValue() < m_FenceValues[m_CurrentBackBufferIndex])
    {
        DX_CALL(m_d3d12Fence->SetEventOnCompletion(m_FenceValues[m_CurrentBackBufferIndex], m_FenceEvent));
        ::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
    }
}

void Renderer::Flush()
{
    m_FenceValues[m_CurrentBackBufferIndex] = ++m_FenceValue;
    m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), m_FenceValues[m_CurrentBackBufferIndex]);

    if (m_d3d12Fence->GetCompletedValue() < m_FenceValues[m_CurrentBackBufferIndex])
    {
        DX_CALL(m_d3d12Fence->SetEventOnCompletion(m_FenceValues[m_CurrentBackBufferIndex], m_FenceEvent));
        ::WaitForSingleObject(m_FenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));
    }
}

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

void Renderer::CreateAdapter()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	DX_CALL(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0,
				__uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			DX_CALL(dxgiAdapter1.As(&m_dxgiAdapter));
		}
	}
}

void Renderer::CreateDevice()
{
    DX_CALL(D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;

    if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue)))
    {
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
        pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

        // Suppress whole categories of messages
        //D3D12_MESSAGE_CATEGORY categories[] = {};

        // Suppress messages based on their severity level
        D3D12_MESSAGE_SEVERITY severities[] =
        {
            D3D12_MESSAGE_SEVERITY_INFO
        };

        // Suppress individual messages by their ID
        D3D12_MESSAGE_ID denyIds[] =
        {
            D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
            D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
        };

        D3D12_INFO_QUEUE_FILTER newFilter = {};
        //newFilter.DenyList.NumCategories = _countof(categories);
        //newFilter.DenyList.pCategoryList = categories;
        newFilter.DenyList.NumSeverities = _countof(severities);
        newFilter.DenyList.pSeverityList = severities;
        newFilter.DenyList.NumIDs = _countof(denyIds);
        newFilter.DenyList.pIDList = denyIds;

        DX_CALL(pInfoQueue->PushStorageFilter(&newFilter));
    }
#endif
}

void Renderer::CreateDescriptorHeap()
{
    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
    {
        D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
        heapDesc.NumDescriptors = 256;
        heapDesc.Type = static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i);

        DX_CALL(m_d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap[i])));
        m_DescriptorSize[i] = m_d3d12Device->GetDescriptorHandleIncrementSize(heapDesc.Type);
    }
}

void Renderer::CreateCommandQueue()
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
    queueDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    DX_CALL(m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
    DX_CALL(m_d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));

    m_FenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
    assert(m_FenceEvent && "Failed to create fence event.");

    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        DX_CALL(m_d3d12Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_d3d12CommandAllocators[i])));
    }

    DX_CALL(m_d3d12Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_d3d12CommandAllocators[0].Get(),
        nullptr, IID_PPV_ARGS(&m_d3d12CommandList)));
    DX_CALL(m_d3d12CommandList->Close());
}

void Renderer::CreateSwapChain(HWND hWnd)
{
    ComPtr<IDXGIFactory> dxgiFactory;
    ComPtr<IDXGIFactory5> dxgiFactory5;

    DX_CALL(m_dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));
    DX_CALL(dxgiFactory.As(&dxgiFactory5));

    BOOL allowTearing = false;
    if (SUCCEEDED(dxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(BOOL))))
    {
        m_TearingSupported = (allowTearing == TRUE);
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_Resolution.x;
    swapChainDesc.Height = m_Resolution.y;
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
    DX_CALL(dxgiFactory5->CreateSwapChainForHwnd(m_d3d12CommandQueue.Get(), hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
    DX_CALL(dxgiSwapChain1.As(&m_dxgiSwapChain));

    DX_CALL(dxgiFactory5->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}

void Renderer::CreateDepthBuffer()
{
    D3D12_CLEAR_VALUE clearValue = {};
    clearValue.Format = DXGI_FORMAT_D32_FLOAT;
    clearValue.DepthStencil = { 1.0f, 0 };

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT,
        m_Resolution.x, m_Resolution.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &clearValue,
        IID_PPV_ARGS(&m_DepthBuffer)
    ));

    m_d3d12Device->CreateDepthStencilView(m_DepthBuffer.Get(), nullptr, m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());
}

void Renderer::CreateRootSignature()
{
    D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
        D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

    //CD3DX12_DESCRIPTOR_RANGE1 descRanges[1] = {};
    //descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    //rootParameters[1].InitAsDescriptorTable(1, descRanges, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Init(0);

    CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
    rootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], _countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);
    
    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    DX_CALL(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &serializedRootSig, &errorBlob));

    DX_CALL(m_d3d12Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_d3d12RootSignature)));
}

void Renderer::CreatePipelineState()
{
    UINT compileFlags = 0;
#ifdef _DEBUG
    compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

    ComPtr<ID3DBlob> errorBlob;
    ComPtr<ID3DBlob> vsBlob;
    ComPtr<ID3DBlob> psBlob;

    DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Default_VS.hlsl", nullptr, nullptr, "main", "vs_5_1", compileFlags, 0, &vsBlob, &errorBlob));
    DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Default_PS.hlsl", nullptr, nullptr, "main", "ps_5_1", compileFlags, 0, &psBlob, &errorBlob));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.BlendEnable = TRUE;
    rtBlendDesc.LogicOpEnable = FALSE;
    rtBlendDesc.SrcBlend = D3D12_BLEND_ONE;
    rtBlendDesc.DestBlend = D3D12_BLEND_ZERO;
    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_ONE;
    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_ZERO;
    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = TRUE;
    blendDesc.IndependentBlendEnable = FALSE;
    blendDesc.RenderTarget[0] = rtBlendDesc;

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vsBlob.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(psBlob.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.BlendState = blendDesc;
    psoDesc.DepthStencilState.DepthEnable = TRUE;
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
    psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
    psoDesc.DepthStencilState.StencilEnable = FALSE;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;
    psoDesc.pRootSignature = m_d3d12RootSignature.Get();

    DX_CALL(m_d3d12Device->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_d3d12PipelineState)));
}

void Renderer::ResizeBackBuffers()
{
    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        m_BackBuffers[i].Reset();
        m_FenceValues[i] = m_FenceValues[m_CurrentBackBufferIndex];
    }

    DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
    DX_CALL(m_dxgiSwapChain->GetDesc(&swapChainDesc));
    DX_CALL(m_dxgiSwapChain->ResizeBuffers(s_BackBufferCount, m_Resolution.x, m_Resolution.y,
        swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
}

void Renderer::UpdateRenderTargetViews()
{
    auto rtvDescriptorSize = m_d3d12Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart());

    for (uint32_t i = 0; i < s_BackBufferCount; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer;
        DX_CALL(m_dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));

        m_d3d12Device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
        m_BackBuffers[i] = backBuffer;

        rtvHandle.Offset(rtvDescriptorSize);
    }
}

void Renderer::TrackResource(ComPtr<ID3D12Resource> resource)
{
    m_TrackedResources.emplace_back(resource);
}

void Renderer::ReleaseTrackedResources()
{
    m_TrackedResources.clear();
}
