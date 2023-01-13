#include "Pch.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/Buffer.h"
#include "Graphics/Shader.h"
#include "Graphics/RasterPass.h"
#include "Graphics/FrameBuffer.h"
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
    std::unique_ptr<RasterPass> RasterPass;
    std::shared_ptr<FrameBuffer> FrameBuffer;

    DebugRendererSettings DebugRenderSettings;
    DebugRendererStatistics DebugRenderStatistics;

    std::vector<LineVertex> LineVertexData;
    std::unique_ptr<Buffer> LineBuffer;

    glm::mat4 CameraViewProjection = glm::identity<glm::mat4>();

    uint32_t CurrentBackBufferIndex = 0;
};

static InternalDebugRendererData s_Data;

void DebugRenderer::Initialize(uint32_t width, uint32_t height)
{
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

    s_Data.FrameBuffer->ClearAttachments();
}

void DebugRenderer::Render()
{
    SCOPED_TIMER("DebugRenderer::Render");

    if (!s_Data.DebugRenderSettings.DrawLines || s_Data.LineVertexData.size() == 0)
        return;

    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Set viewports, scissor rects and render targets
    commandList->SetRenderPassBindables(*s_Data.RasterPass);

    const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();
    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(renderSettings.RenderResolution.x),
        static_cast<float>(renderSettings.RenderResolution.y), 0.0f, 1.0f);
    CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

    commandList->SetViewports(1, &viewport);
    commandList->SetScissorRects(1, &scissorRect);

    auto& finalColorOutput = Renderer::GetFinalColorOutput();
    auto& finalDepthOutput = Renderer::GetFinalDepthOutput();

    commandList->Transition(finalColorOutput, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->Transition(finalDepthOutput, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = finalColorOutput.GetDescriptor(DescriptorType::RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = finalDepthOutput.GetDescriptor(DescriptorType::DSV);
    commandList->SetRenderTargets(1, &rtv, &dsv);

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
    s_Data.FrameBuffer->Resize(width, height);
}

void DebugRenderer::MakeRenderPasses()
{
    const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

    {
        // Debug line render pass
        RasterPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
        desc.DepthEnabled = true;
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Data.RasterPass = std::make_unique<RasterPass>("Debug line", desc);
    }
}

void DebugRenderer::MakeBuffers()
{
    s_Data.LineBuffer = std::make_unique<Buffer>("Line vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 10000, sizeof(LineVertex)));
}

void DebugRenderer::MakeFrameBuffers()
{
    auto& renderSettings = Renderer::GetSettings();

    FrameBufferDesc debugDesc = {};
    debugDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
        TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
    debugDesc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
        TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
    s_Data.FrameBuffer = std::make_shared<FrameBuffer>("Debug line frame buffer", debugDesc);
}
