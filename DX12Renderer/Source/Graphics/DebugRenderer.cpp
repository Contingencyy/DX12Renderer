#include "Pch.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/RenderBackend.h"
#include "Graphics/Buffer.h"
#include "Graphics/RenderPass.h"
#include "Graphics/CommandList.h"

#include <imgui/imgui.h>

static DebugRenderer* s_Instance = nullptr;

void DebugRenderer::Initialize(uint32_t width, uint32_t height)
{
    if (!s_Instance)
        s_Instance = new DebugRenderer();

    s_Instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    s_Instance->m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    MakeRenderPasses();
    MakeBuffers();
}

void DebugRenderer::Finalize()
{
    delete s_Instance;
}

void DebugRenderer::BeginScene(const Camera& sceneCamera)
{
    SCOPED_TIMER("DebugRenderer::BeginFrame");

    s_Instance->m_CameraViewProjection = sceneCamera.GetViewProjection();

    auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    s_Instance->m_RenderPass->ClearViews(commandList);
    RenderBackend::Get().ExecuteCommandList(commandList);
}

void DebugRenderer::Render()
{
    SCOPED_TIMER("DebugRenderer::Render");

    if (!s_Instance->m_DebugRenderSettings.DrawLines || s_Instance->m_LineVertexData.size() == 0)
        return;

    auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto& colorAttachment = Renderer::GetFinalColorOutput();
    auto& depthAttachment = Renderer::GetFinalDepthOutput();

    auto rtv = colorAttachment.GetRenderTargetDepthStencilView();
    auto dsv = depthAttachment.GetRenderTargetDepthStencilView();

    // Set viewports, scissor rects and render targets
    commandList->SetViewports(1, &s_Instance->m_Viewport);
    commandList->SetScissorRects(1, &s_Instance->m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set pipeline state and root signature
    commandList->SetPipelineState(s_Instance->m_RenderPass->GetPipelineState());

    commandList->SetRootConstants(0, 16, &s_Instance->m_CameraViewProjection, 0);

    s_Instance->m_LineBuffer->SetBufferData(&s_Instance->m_LineVertexData[0], sizeof(LineVertex) * s_Instance->m_LineVertexData.size());
    commandList->SetVertexBuffers(0, 1, *s_Instance->m_LineBuffer);

    commandList->Draw(s_Instance->m_LineVertexData.size(), 1);

    s_Instance->m_DebugRenderStatistics.DrawCallCount++;
    s_Instance->m_DebugRenderStatistics.LineCount += s_Instance->m_LineVertexData.size() / 2;

    RenderBackend::Get().ExecuteCommandList(commandList);
}

void DebugRenderer::OnImGuiRender()
{
    ImGui::Text("Debug renderer");
    ImGui::Text("Draw calls: %u", s_Instance->m_DebugRenderStatistics.DrawCallCount);
    ImGui::Text("Line count: %u", s_Instance->m_DebugRenderStatistics.LineCount);
    ImGui::Checkbox("Draw lines", &s_Instance->m_DebugRenderSettings.DrawLines);
}

void DebugRenderer::EndScene()
{
    SCOPED_TIMER("DebugRenderer::EndFrame");

    s_Instance->m_LineVertexData.clear();

    s_Instance->m_DebugRenderStatistics.Reset();
}

void DebugRenderer::Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color)
{
    s_Instance->m_LineVertexData.emplace_back(lineStart, color);
    s_Instance->m_LineVertexData.emplace_back(lineEnd, color);
}

void DebugRenderer::Resize(uint32_t width, uint32_t height)
{
    const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

    s_Instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(renderSettings.Resolution.x),
        static_cast<float>(renderSettings.Resolution.y), 0.0f, 1.0f);

    s_Instance->m_RenderPass->Resize(static_cast<float>(renderSettings.Resolution.x), static_cast<float>(renderSettings.Resolution.y));
}

void DebugRenderer::MakeRenderPasses()
{
    const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

    {
        // Debug line render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            renderSettings.Resolution.x, renderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            renderSettings.Resolution.x, renderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Instance->m_RenderPass = std::make_unique<RenderPass>("Debug line", desc);
    }
}

void DebugRenderer::MakeBuffers()
{
    s_Instance->m_LineBuffer = std::make_unique<Buffer>("Line vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 10000, sizeof(LineVertex)));
}
