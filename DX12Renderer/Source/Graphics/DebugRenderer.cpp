#include "Pch.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderState.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/Buffer.h"
#include "Graphics/Shader.h"
#include "Graphics/RasterPass.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/SwapChain.h"

#include <imgui/imgui.h>

struct DebugRendererSettings
{
    bool DrawLines = false;
};

struct LineVertex
{
    glm::vec3 Position;
    glm::vec4 Color;
};

struct InternalDebugRendererData
{
    std::unique_ptr<RasterPass> RasterPass;
    DebugRendererSettings DebugRenderSettings;

    glm::mat4 CameraViewProjection = glm::identity<glm::mat4>();
    std::size_t DebugLineAt = 0;
};

static InternalDebugRendererData s_Data;

namespace DebugRenderer
{

    void CreateRenderPasses()
    {
        {
            // Debug line render pass
            RasterPassDesc desc;
            desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
            desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";

            desc.ColorAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.ColorAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
            desc.ColorAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.ColorAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;

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

    void CreateBuffers()
    {
        BufferDesc debugLineBufferDesc = {};
        debugLineBufferDesc.Usage = BufferUsage::BUFFER_USAGE_UPLOAD;
        debugLineBufferDesc.NumElements = g_RenderState.MAX_DEBUG_LINES * 2 * g_RenderState.BACK_BUFFER_COUNT;
        debugLineBufferDesc.ElementSize = sizeof(LineVertex);
        debugLineBufferDesc.DebugName = "Debug line buffer";

        g_RenderState.DebugLineBuffer = std::make_unique<Buffer>(debugLineBufferDesc);
    }

}

void DebugRenderer::Initialize(uint32_t width, uint32_t height)
{
    CreateRenderPasses();
    CreateBuffers();
}

void DebugRenderer::Finalize()
{
}

void DebugRenderer::BeginScene(const Camera& sceneCamera)
{
    // Set the camera view projection
    s_Data.CameraViewProjection = sceneCamera.GetViewProjection();
}

void DebugRenderer::Render()
{
    SCOPED_TIMER("DebugRenderer::Render");

    if (!s_Data.DebugRenderSettings.DrawLines || s_Data.DebugLineAt == 0)
        return;

    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    // Set viewports, scissor rects and render targets
    commandList->SetRenderPassBindables(*s_Data.RasterPass);

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(g_RenderState.Settings.RenderResolution.x),
        static_cast<float>(g_RenderState.Settings.RenderResolution.y), 0.0f, 1.0f);
    CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

    commandList->SetViewports(1, &viewport);
    commandList->SetScissorRects(1, &scissorRect);

    commandList->Transition(*g_RenderState.SDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

    D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_RenderState.SDRColorTarget->GetDescriptor(DescriptorType::RTV);
    D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    commandList->SetRootConstants(0, 16, &s_Data.CameraViewProjection, 0);

    commandList->SetVertexBuffers(0, 1, *g_RenderState.DebugLineBuffer);

    commandList->Draw(s_Data.DebugLineAt, 1);

    RenderBackend::ExecuteCommandList(commandList);
}

void DebugRenderer::OnImGuiRender()
{
    if (ImGui::CollapsingHeader("Settings"))
    {
        ImGui::Indent(10.0f);
        ImGui::Checkbox("Draw lines", &s_Data.DebugRenderSettings.DrawLines);
        ImGui::Unindent(10.0f);
    }

    if (ImGui::CollapsingHeader("Stats"))
    {
        ImGui::Indent(10.0f);
        ImGui::Text("Line count: %u", s_Data.DebugLineAt);
        ImGui::Unindent(10.0f);
    }
}

void DebugRenderer::EndScene()
{
    s_Data.DebugLineAt = 0;
}

void DebugRenderer::Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color)
{
    if (s_Data.DebugLineAt > g_RenderState.MAX_DEBUG_LINES)
        return;

    LineVertex vertices[2] = {
        { lineStart, color },
        { lineEnd, color }
    };
    g_RenderState.DebugLineBuffer->SetBufferDataAtOffset(&vertices, 2 * sizeof(LineVertex), s_Data.DebugLineAt * 2 * sizeof(LineVertex));
    s_Data.DebugLineAt++;
}

void DebugRenderer::SubmitAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color)
{
    Submit(min, glm::vec3(max.x, min.y, min.z), color);
    Submit(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, max.y, min.z), color);
    Submit(glm::vec3(max.x, max.y, min.z), glm::vec3(min.x, max.y, min.z), color);
    Submit(glm::vec3(min.x, max.y, min.z), min, color);

    Submit(glm::vec3(min.x, min.y, max.z), glm::vec3(max.x, min.y, max.z), color);
    Submit(glm::vec3(max.x, min.y, max.z), glm::vec3(max.x, max.y, max.z), color);
    Submit(glm::vec3(max.x, max.y, max.z), glm::vec3(min.x, max.y, max.z), color);
    Submit(glm::vec3(min.x, max.y, max.z), glm::vec3(min.x, min.y, max.z), color);

    Submit(min, glm::vec3(min.x, min.y, max.z), color);
    Submit(glm::vec3(min.x, max.y, min.z), glm::vec3(min.x, max.y, max.z), color);
    Submit(glm::vec3(max.x, min.y, min.z), glm::vec3(max.x, min.y, max.z), color);
    Submit(glm::vec3(max.x, max.y, min.z), glm::vec3(max.x, max.y, max.z), color);
}
