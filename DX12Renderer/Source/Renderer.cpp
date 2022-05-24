#include "Pch.h"
#include "Renderer.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/Buffer.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

struct Vertex
{
    glm::vec3 Position;
    //glm::vec2 TexCoord;
};

//std::vector<Vertex> QuadVertices = {
//    { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 1.0f) },
//    { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec2(0.0f, 0.0f) },
//    { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(1.0f, 0.0f) },
//    { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 1.0f) }
//};

std::vector<Vertex> QuadVertices = {
    { glm::vec3(-0.5f, -0.5f, 0.0f) },
    { glm::vec3(-0.5f, 0.5f, 0.0f) },
    { glm::vec3(0.5f, 0.5f, 0.0f) },
    { glm::vec3(0.5f, -0.5f, 0.0f) }
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

void Renderer::Initialize(uint32_t width, uint32_t height)
{
	m_RenderSettings.Resolution.x = width;
	m_RenderSettings.Resolution.y = height;

	EnableDebugLayer();

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x), static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);
	m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    CreateAdapter();
    CreateDevice();

    m_CommandQueueDirect = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueueCopy = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

    CreateDescriptorHeap();
    CreateSwapChain(Application::Get().GetWindow()->GetHandle());
    CreateDepthBuffer();

    CreateRootSignature();
    CreatePipelineState();

    UpdateRenderTargetViews();

    m_QuadVertexBuffer = std::make_unique<Buffer>(BufferDesc(), QuadVertices.size(), sizeof(QuadVertices[0]));
    Buffer quadVBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadVertexBuffer->GetAlignedSize());
    Application::Get().GetRenderer()->CopyBuffer(quadVBUploadBuffer, *m_QuadVertexBuffer.get(), &QuadVertices[0]);

    m_QuadIndexBuffer = std::make_unique<Buffer>(BufferDesc(), QuadIndices.size(), sizeof(QuadIndices[0]));
    Buffer quadIBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadIndexBuffer->GetAlignedSize());
    Application::Get().GetRenderer()->CopyBuffer(quadIBUploadBuffer, *m_QuadIndexBuffer.get(), &QuadIndices[0]);

}

void Renderer::Finalize()
{
    Flush();
}

void Renderer::BeginFrame(const Camera& camera)
{
    m_SceneData.Camera = camera;

    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    // Transition back buffer to render target state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
        m_CurrentBackBufferIndex, m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());

    // Clear render target and depth stencil view
    float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor);
    commandList->ClearDepthStencilView(dsv);

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    auto commandList = m_CommandQueueDirect->GetCommandList();

    CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
        m_CurrentBackBufferIndex, m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
    CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());

    commandList->SetViewports(1, &m_Viewport);
    commandList->SetScissorRects(1, &m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    commandList->SetPipelineState(m_d3d12PipelineState.Get());
    commandList->SetRootSignature(m_d3d12RootSignature.Get());
    commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    glm::mat4 viewProjection = m_SceneData.Camera.GetViewProjection();
    commandList->SetRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);

    commandList->SetVertexBuffers(0, 1, *m_QuadVertexBuffer.get());
    commandList->SetIndexBuffer(*m_QuadIndexBuffer.get());

    for (auto& data : m_QuadRenderData)
    {
        commandList->SetVertexBuffers(1, 1, *data.InstanceBuffer);
        commandList->DrawIndexed(QuadIndices.size(), data.NumQuads);

        m_RenderStatistics.DrawCallCount++;
        m_RenderStatistics.QuadCount += data.NumQuads;
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::GUIRender()
{
    ImGui::Begin("Profiler");

    float lastFrameDuration = Application::Get().GetLastFrameTime() * 1000.0f;

    ImGui::Text("Resolution: %ux%u", m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Frametime: %.3f ms", lastFrameDuration);
    ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / lastFrameDuration));
    ImGui::Text("Draw calls: %u", m_RenderStatistics.DrawCallCount);
    ImGui::Text("Quad count: %u", m_RenderStatistics.QuadCount);

    ImGui::End();
}

void Renderer::EndFrame()
{
    Present();

    m_QuadRenderData.clear();

    m_RenderStatistics.DrawCallCount = 0;
    m_RenderStatistics.QuadCount = 0;
}

void Renderer::DrawQuads(std::shared_ptr<Buffer> instanceBuffer, std::size_t numQuads)
{
    m_QuadRenderData.push_back({ instanceBuffer, numQuads });
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (m_RenderSettings.Resolution.x != width || m_RenderSettings.Resolution.y != height)
    {
        m_RenderSettings.Resolution.x = std::max(1u, width);
        m_RenderSettings.Resolution.y = std::max(1u, height);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x),
            static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);

        Flush();

        ResizeBackBuffers();
        CreateDepthBuffer();

        UpdateRenderTargetViews();
    }
}

void Renderer::CreateBuffer(ComPtr<ID3D12Resource>& resource, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size)
{
    CD3DX12_HEAP_PROPERTIES heapProps(bufferType);
    CD3DX12_RESOURCE_DESC heapDesc(CD3DX12_RESOURCE_DESC::Buffer(size));

    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &heapDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&resource)
    ));
}

void Renderer::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void Renderer::CreateTexture(ComPtr<ID3D12Resource>& resource, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, std::size_t size)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&resource)
    ));
}

void Renderer::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyTexture(intermediateBuffer, destTexture, textureData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

D3D12_CPU_DESCRIPTOR_HANDLE Renderer::AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    CD3DX12_CPU_DESCRIPTOR_HANDLE descriptorHandle(m_d3d12DescriptorHeap[type]->GetCPUDescriptorHandleForHeapStart(),
        m_DescriptorOffsets[type], m_DescriptorSize[type]);
    m_DescriptorOffsets[type]++;

    return descriptorHandle;
}

void Renderer::Present()
{
    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto& backBuffer = m_BackBuffers[m_CurrentBackBufferIndex];

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
    commandList->ResourceBarrier(1, &barrier);
    m_CommandQueueDirect->ExecuteCommandList(commandList);

    unsigned int syncInterval = m_RenderSettings.VSync ? 1 : 0;
    unsigned int presentFlags = m_RenderSettings.TearingSupported && !m_RenderSettings.VSync ? DXGI_PRESENT_ALLOW_TEARING : 0;
    DX_CALL(m_dxgiSwapChain->Present(syncInterval, presentFlags));

    m_FenceValues[m_CurrentBackBufferIndex] = m_CommandQueueDirect->Signal();

    m_CurrentBackBufferIndex = m_dxgiSwapChain->GetCurrentBackBufferIndex();
    m_CommandQueueDirect->WaitForFenceValue(m_FenceValues[m_CurrentBackBufferIndex]);

    m_CommandQueueDirect->ResetCommandLists();
    m_CommandQueueCopy->ResetCommandLists();
}

void Renderer::Flush()
{
    m_CommandQueueDirect->Flush();
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

        if (i == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV)
            heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

        DX_CALL(m_d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap[i])));
        m_DescriptorSize[i] = m_d3d12Device->GetDescriptorHandleIncrementSize(heapDesc.Type);

        m_DescriptorOffsets[i] = 0;
    }
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
        m_RenderSettings.TearingSupported = (allowTearing == TRUE);
    }

    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.Width = m_RenderSettings.Resolution.x;
    swapChainDesc.Height = m_RenderSettings.Resolution.y;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.Stereo = FALSE;
    swapChainDesc.SampleDesc = { 1, 0 };
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.BufferCount = s_BackBufferCount;
    swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
    swapChainDesc.Flags = m_RenderSettings.TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> dxgiSwapChain1;
    DX_CALL(dxgiFactory5->CreateSwapChainForHwnd(m_CommandQueueDirect->GetD3D12CommandQueue().Get(),
        hWnd, &swapChainDesc, nullptr, nullptr, &dxgiSwapChain1));
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
        m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y, 1, 1, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

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

    CD3DX12_DESCRIPTOR_RANGE1 descRanges[1] = {};
    descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    CD3DX12_ROOT_PARAMETER1 rootParameters[1] = {};
    //CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    //rootParameters[1].InitAsDescriptorTable(1, descRanges, D3D12_SHADER_VISIBILITY_PIXEL);

    CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
    staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
    staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
    staticSamplers[0].MipLODBias = 0;
    staticSamplers[0].MaxAnisotropy = 0;
    staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
    staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
    staticSamplers[0].MinLOD = 0.0f;
    staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
    staticSamplers[0].ShaderRegister = 0;
    staticSamplers[0].RegisterSpace = 0;
    staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

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

    //DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Default_VS.hlsl", nullptr, nullptr, "main", "vs_5_1", compileFlags, 0, &vsBlob, &errorBlob));
    //DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Default_PS.hlsl", nullptr, nullptr, "main", "ps_5_1", compileFlags, 0, &psBlob, &errorBlob));
    DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Particle_VS.hlsl", nullptr, nullptr, "main", "vs_5_1", compileFlags, 0, &vsBlob, &errorBlob));
    DX_CALL(D3DCompileFromFile(L"Resources/Shaders/Particle_PS.hlsl", nullptr, nullptr, "main", "ps_5_1", compileFlags, 0, &psBlob, &errorBlob));

    /*D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };*/
    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
    };

    D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
    rtBlendDesc.BlendEnable = TRUE;
    rtBlendDesc.LogicOpEnable = FALSE;
    rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
    rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
    rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
    rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
    rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
    rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

    D3D12_BLEND_DESC blendDesc = {};
    blendDesc.AlphaToCoverageEnable = FALSE;
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
    DX_CALL(m_dxgiSwapChain->ResizeBuffers(s_BackBufferCount, m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y,
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
