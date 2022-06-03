#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/SwapChain.h"
#include "Graphics/Buffer.h"
#include "Graphics/Shader.h"
#include "Graphics/Model.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

struct Vertex
{
    glm::vec3 Position;
    glm::vec2 TexCoord;
};

std::vector<Vertex> QuadVertices = {
    { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 1.0f) },
    { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec2(0.0f, 0.0f) },
    { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(1.0f, 0.0f) },
    { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 1.0f) }
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

    m_SwapChain = std::make_shared<SwapChain>(Application::Get().GetWindow()->GetHandle(), width, height);
    CreateDescriptorHeap();

    m_QuadVertexBuffer = std::make_unique<Buffer>(BufferDesc(), QuadVertices.size(), sizeof(QuadVertices[0]));
    Buffer quadVBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadVertexBuffer->GetByteSize());
    Application::Get().GetRenderer()->CopyBuffer(quadVBUploadBuffer, *m_QuadVertexBuffer.get(), &QuadVertices[0]);

    m_QuadIndexBuffer = std::make_unique<Buffer>(BufferDesc(), QuadIndices.size(), sizeof(QuadIndices[0]));
    Buffer quadIBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadIndexBuffer->GetByteSize());
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
    auto backBuffer = m_SwapChain->GetBackBuffer();
    auto depthBuffer = m_SwapChain->GetDepthBuffer();

    // Transition back buffer to render target state
    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer->GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->ResourceBarrier(1, &barrier);

    auto rtv = backBuffer->GetDescriptorHandle();
    auto dsv = depthBuffer->GetDescriptorHandle();

    // Clear render target and depth stencil view
    float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
    commandList->ClearRenderTargetView(rtv, clearColor);
    commandList->ClearDepthStencilView(dsv);

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto backBuffer = m_SwapChain->GetBackBuffer();
    auto depthBuffer = m_SwapChain->GetDepthBuffer();

    auto rtv = backBuffer->GetDescriptorHandle();
    auto dsv = depthBuffer->GetDescriptorHandle();

    commandList->SetViewports(1, &m_Viewport);
    commandList->SetScissorRects(1, &m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    for (auto& model : m_ModelDrawData)
    {
        commandList->SetPipelineState(model->GetPipelineState().Get());
        commandList->SetRootSignature(model->GetRootSignature().Get());
        commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

        glm::mat4 viewProjection = m_SceneData.Camera.GetViewProjection();
        commandList->SetRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);

        commandList->SetVertexBuffers(0, 1, *model->GetBuffer(0).get());
        commandList->SetVertexBuffers(1, 1, *model->GetBuffer(1).get());
        commandList->SetVertexBuffers(2, 1, *model->GetBuffer(2).get());
        commandList->SetIndexBuffer(*model->GetBuffer(3).get());

        commandList->SetDescriptorHeap(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get());
        commandList->SetShaderResourceView(m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV].Get(), model->GetTexture(0)->GetDescriptorHandle());

        commandList->DrawIndexed(model->GetBuffer(3)->GetNumElements(), 1);

        m_RenderStatistics.DrawCallCount++;
        m_RenderStatistics.TriangleCount += model->GetBuffer(3)->GetNumElements() / 3;
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::ImGuiRender()
{
    ImGui::Begin("Profiler");

    float lastFrameDuration = Application::Get().GetLastFrameTime() * 1000.0f;

    ImGui::Text("Resolution: %ux%u", m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Frametime: %.3f ms", lastFrameDuration);
    ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / lastFrameDuration));
    ImGui::Text("Draw calls: %u", m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", m_RenderStatistics.TriangleCount);

    ImGui::End();
}

void Renderer::EndFrame()
{
    m_SwapChain->SwapBuffers();

    m_CommandQueueDirect->ResetCommandLists();
    m_CommandQueueCopy->ResetCommandLists();

    m_QuadDrawData.clear();
    m_ModelDrawData.clear();

    m_RenderStatistics.Reset();
}

void Renderer::DrawQuads(std::shared_ptr<Buffer> instanceBuffer, std::shared_ptr<Texture> texture, std::size_t numQuads)
{
    m_QuadDrawData.push_back({ instanceBuffer, texture, numQuads });
}

void Renderer::DrawModel(Model* model)
{
    m_ModelDrawData.push_back(model);
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

        m_SwapChain->Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    }
}

void Renderer::CreateBuffer(Buffer& buffer, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size)
{
    CD3DX12_HEAP_PROPERTIES heapProps(bufferType);
    CD3DX12_RESOURCE_DESC heapDesc(CD3DX12_RESOURCE_DESC::Buffer(size));

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &heapDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    buffer.SetD3D12Resource(d3d12Resource);
}

void Renderer::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void Renderer::CreateTexture(Texture& texture, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        initialState,
        clearValue,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    texture.SetD3D12Resource(d3d12Resource);
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
