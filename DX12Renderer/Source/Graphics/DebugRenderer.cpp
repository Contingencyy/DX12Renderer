#include "Pch.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/Buffer.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/SwapChain.h"

#include <imgui/imgui.h>

struct DebugRendererSettings
{
    bool DrawLines = false;
};

struct DebugRendererStatistics
{
    uint32_t DrawCallCount = 0;
    uint32_t LineCount = 0;

    void Reset()
    {
        DrawCallCount = 0;
        LineCount = 0;
    }
};

struct LineVertex
{
    LineVertex(const glm::vec3& position, const glm::vec4& color)
        : Position(position), Color(color) {}

    glm::vec3 Position;
    glm::vec4 Color;
};

struct InternalDebugRendererData
{
    std::unique_ptr<RenderPass> RenderPass;
    std::vector<std::shared_ptr<Texture>> FrameBuffers[3];

    DebugRendererSettings DebugRenderSettings;
    DebugRendererStatistics DebugRenderStatistics;

    D3D12_VIEWPORT Viewport = D3D12_VIEWPORT();
    D3D12_RECT ScissorRect = D3D12_RECT();

    std::vector<LineVertex> LineVertexData;
    std::unique_ptr<Buffer> LineBuffer;

    glm::mat4 CameraViewProjection = glm::identity<glm::mat4>();

    uint32_t CurrentBackBufferIndex = 0;
};

static InternalDebugRendererData s_Data;

void DebugRenderer::Initialize(uint32_t width, uint32_t height)
{
    s_Data.Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width), static_cast<float>(height), 0.0f, 1.0f);
    s_Data.ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    MakeRenderPasses();
    MakeBuffers();
    MakeFrameBuffers();
}

void DebugRenderer::Finalize()
{
}

void DebugRenderer::BeginScene(const Camera& sceneCamera)
{
    SCOPED_TIMER("DebugRenderer::BeginFrame");

    s_Data.CurrentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();

    s_Data.CameraViewProjection = sceneCamera.GetViewProjection();

    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    
    for (auto& frameBuffer : s_Data.FrameBuffers[s_Data.CurrentBackBufferIndex])
    {
        const TextureDesc& frameBufferDesc = frameBuffer->GetTextureDesc();
        if (frameBufferDesc.Usage & TextureUsage::TEXTURE_USAGE_RENDER_TARGET)
            commandList->ClearRenderTargetView(frameBuffer->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(frameBufferDesc.ClearColor));
        else if (frameBufferDesc.Usage & TextureUsage::TEXTURE_USAGE_DEPTH)
            commandList->ClearDepthStencilView(frameBuffer->GetDescriptor(DescriptorType::DSV), frameBufferDesc.ClearDepthStencil.x);
    }

    RenderBackend::ExecuteCommandList(commandList);
}

void DebugRenderer::Render()
{
    SCOPED_TIMER("DebugRenderer::Render");

    if (!s_Data.DebugRenderSettings.DrawLines || s_Data.LineVertexData.size() == 0)
        return;

    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto& colorAttachment = Renderer::GetFinalColorOutput();
    auto& depthAttachment = Renderer::GetFinalDepthOutput();

    auto rtv = colorAttachment.GetDescriptor(DescriptorType::RTV);
    auto dsv = depthAttachment.GetDescriptor(DescriptorType::DSV);

    // Set viewports, scissor rects and render targets
    commandList->SetViewports(1, &s_Data.Viewport);
    commandList->SetScissorRects(1, &s_Data.ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set pipeline state and root signature
    commandList->SetPipelineState(s_Data.RenderPass->GetPipelineState());

    commandList->SetRootConstants(0, 16, &s_Data.CameraViewProjection, 0);

    s_Data.LineBuffer->SetBufferData(&s_Data.LineVertexData[0], sizeof(LineVertex) * s_Data.LineVertexData.size());
    commandList->SetVertexBuffers(0, 1, *s_Data.LineBuffer);

    commandList->Draw(static_cast<uint32_t>(s_Data.LineVertexData.size()), 1);

    s_Data.DebugRenderStatistics.DrawCallCount++;
    s_Data.DebugRenderStatistics.LineCount += static_cast<uint32_t>(s_Data.LineVertexData.size()) / 2;

    RenderBackend::ExecuteCommandList(commandList);
}

void DebugRenderer::OnImGuiRender()
{
    ImGui::Text("Debug renderer");
    ImGui::Text("Draw calls: %u", s_Data.DebugRenderStatistics.DrawCallCount);
    ImGui::Text("Line count: %u", s_Data.DebugRenderStatistics.LineCount);
    ImGui::Checkbox("Draw lines", &s_Data.DebugRenderSettings.DrawLines);
}

void DebugRenderer::EndScene()
{
    SCOPED_TIMER("DebugRenderer::EndFrame");

    s_Data.LineVertexData.clear();

    s_Data.DebugRenderStatistics.Reset();
}

void DebugRenderer::Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color)
{
    s_Data.LineVertexData.emplace_back(lineStart, color);
    s_Data.LineVertexData.emplace_back(lineEnd, color);
}

void DebugRenderer::Resize(uint32_t width, uint32_t height)
{
    s_Data.Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(width),
        static_cast<float>(height), 0.0f, 1.0f);

    for (auto& frameBuffers : s_Data.FrameBuffers)
    {
        for (auto& frameBuffer : frameBuffers)
            frameBuffer->Resize(width, height);
    }
}

void DebugRenderer::MakeRenderPasses()
{
    const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

    {
        // Debug line render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
        desc.DepthEnabled = true;
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Data.RenderPass = std::make_unique<RenderPass>("Debug line", desc);
    }
}

void DebugRenderer::MakeBuffers()
{
    s_Data.LineBuffer = std::make_unique<Buffer>("Line vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 10000, sizeof(LineVertex)));
}

void DebugRenderer::MakeFrameBuffers()
{
}
