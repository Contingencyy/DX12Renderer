#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/Buffer.h"
#include "Graphics/Shader.h"
#include "Graphics/Model.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RootSignature.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

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

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x), static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);
	m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    m_Device = std::make_shared<Device>();

    m_CommandQueueDirect = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueueCopy = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

    m_SwapChain = std::make_shared<SwapChain>(Application::Get().GetWindow()->GetHandle(), width, height);

    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_DescriptorHeaps[i] = std::make_shared<DescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));
}

void Renderer::Finalize()
{
    Flush();
}

void Renderer::BeginFrame(const Camera& camera)
{
    m_SceneData.Camera = camera;

    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto colorTarget = m_SwapChain->GetColorTarget();
    auto depthBuffer = m_SwapChain->GetDepthBuffer();

    auto rtv = colorTarget->GetDescriptorHandle();
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
    auto colorTarget = m_SwapChain->GetColorTarget();
    auto depthBuffer = m_SwapChain->GetDepthBuffer();

    auto rtv = colorTarget->GetDescriptorHandle();
    auto dsv = depthBuffer->GetDescriptorHandle();

    commandList->SetViewports(1, &m_Viewport);
    commandList->SetScissorRects(1, &m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    for (auto& model : m_ModelDrawData)
    {
        commandList->SetPipelineState(*model->GetPipelineState());

        glm::mat4 viewProjection = m_SceneData.Camera.GetViewProjection();
        commandList->SetRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);

        commandList->SetVertexBuffers(0, 1, *model->GetBuffer(0).get());
        commandList->SetVertexBuffers(1, 1, *model->GetBuffer(1).get());
        commandList->SetVertexBuffers(2, 1, *model->GetBuffer(2).get());
        commandList->SetIndexBuffer(*model->GetBuffer(3).get());

        commandList->SetShaderResourceView(1, 0, *model->GetTexture(0).get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->DrawIndexed(model->GetBuffer(3)->GetNumElements(), 1);

        CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(model->GetTexture(0)->GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON);
        commandList->ResourceBarrier(1, &barrier);

        m_RenderStatistics.DrawCallCount++;
        m_RenderStatistics.TriangleCount += model->GetBuffer(3)->GetNumElements() / 3;
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::ImGuiRender()
{
    ImGui::Begin("Renderer");

    ImGui::Text("Resolution: %ux%u", m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Draw calls: %u", m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", m_RenderStatistics.TriangleCount);

    ImGui::End();
}

void Renderer::EndFrame()
{
    m_SwapChain->SwapBuffers();

    m_CommandQueueDirect->ResetCommandLists();
    m_CommandQueueCopy->ResetCommandLists();

    m_ModelDrawData.clear();

    m_RenderStatistics.Reset();
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

        m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Reset();
        m_SwapChain->Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    }
}

void Renderer::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void Renderer::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyTexture(intermediateBuffer, destTexture, textureData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

DescriptorAllocation Renderer::AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return m_DescriptorHeaps[type]->Allocate(numDescriptors);
}

void Renderer::Flush()
{
    m_CommandQueueDirect->Flush();
}
