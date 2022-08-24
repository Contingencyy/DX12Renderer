#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/Buffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Resource/Model.h"
#include "Graphics/RenderPass.h"
#include "Graphics/RenderBackend.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

static Renderer* s_Instance = nullptr;

enum RenderPassType : uint32_t
{
    DEFAULT,
    DEBUG_LINE,
    TONE_MAPPING,
    NUM_RENDER_PASSES = (TONE_MAPPING + 1)
};

struct MeshInstanceData
{
    MeshInstanceData(const glm::mat4& transform, const glm::vec4& color)
        : Transform(transform), Color(color) {}

    glm::mat4 Transform = glm::identity<glm::mat4>();
    glm::vec4 Color = glm::vec4(1.0f);
};

struct MeshDrawData
{
    MeshDrawData(const std::shared_ptr<Mesh>& mesh)
        : Mesh(mesh)
    {
    }

    std::shared_ptr<Mesh> Mesh;
    std::vector<MeshInstanceData> MeshInstanceData;
};

struct SceneData
{
    SceneData() = default;

    glm::mat4 ViewProjection = glm::identity<glm::mat4>();
    glm::vec3 Ambient = glm::vec3(0.0f);
    uint32_t NumPointlights = 0;
};

struct LineVertex
{
    LineVertex(const glm::vec3& position, const glm::vec4& color)
        : Position(position), Color(color) {}

    glm::vec3 Position;
    glm::vec4 Color;
};

struct RendererInternalData
{
    std::unique_ptr<RenderPass> m_RenderPasses[RenderPassType::NUM_RENDER_PASSES];

    D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
    D3D12_RECT m_ScissorRect = D3D12_RECT();

    Renderer::RenderSettings m_RenderSettings;
    Renderer::RenderStatistics m_RenderStatistics;

    std::unordered_map<std::size_t, MeshDrawData> m_MeshDrawData;
    std::unique_ptr<Buffer> m_MeshInstanceBuffer;

    std::vector<PointlightData> m_PointlightDrawData;
    std::unique_ptr<Buffer> m_PointlightBuffer;

    std::vector<LineVertex> m_LineVertexData;
    std::unique_ptr<Buffer> m_LineBuffer;

    std::unique_ptr<Buffer> m_ToneMapVertexBuffer;
    std::unique_ptr<Buffer> m_ToneMapIndexBuffer;

    SceneData m_SceneData;
    std::unique_ptr<Buffer> m_SceneDataConstantBuffer;
};

static RendererInternalData s_Data;

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    if (!s_Instance)
        s_Instance = new Renderer();

    RenderBackend::Get().Initialize(hWnd, width, height);

    s_Data.m_RenderSettings.Resolution.x = width;
    s_Data.m_RenderSettings.Resolution.y = height;

    s_Data.m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.m_RenderSettings.Resolution.x), static_cast<float>(s_Data.m_RenderSettings.Resolution.y), 0.0f, 1.0f);
    s_Data.m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    CreateRenderPasses();
    InitializeBuffers();
}

void Renderer::Finalize()
{
    RenderBackend::Get().Finalize();
}

void Renderer::BeginFrame(const Camera& camera)
{
    SCOPED_TIMER("Renderer::BeginFrame");

    s_Data.m_SceneData.ViewProjection = camera.GetViewProjection();

    auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (auto& renderPass : s_Data.m_RenderPasses)
        renderPass->ClearViews(commandList);

    RenderBackend::Get().ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    /*
    
        Default pipeline
    
    */
    auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto& defaultColorTarget = s_Data.m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
    auto& defaultDepthBuffer = s_Data.m_RenderPasses[RenderPassType::DEFAULT]->GetDepthAttachment();

    auto rtv = defaultColorTarget.GetDescriptorHandle();
    auto dsv = defaultDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList->SetViewports(1, &s_Data.m_Viewport);
    commandList->SetScissorRects(1, &s_Data.m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set pipeline state, root signature, and scene data constant buffer
    commandList->SetPipelineState(s_Data.m_RenderPasses[RenderPassType::DEFAULT]->GetPipelineState());

    s_Data.m_SceneData.Ambient = glm::vec3(0.1f);
    s_Data.m_SceneData.NumPointlights = s_Data.m_PointlightDrawData.size();

    s_Data.m_SceneDataConstantBuffer->SetBufferData(&s_Data.m_SceneData);
    commandList->SetRootConstantBufferView(0, *s_Data.m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

    // Set constant buffer data for point lights
    if (s_Data.m_SceneData.NumPointlights > 0)
    {
        s_Data.m_PointlightBuffer->SetBufferData(&s_Data.m_PointlightDrawData[0], s_Data.m_PointlightDrawData.size() * sizeof(PointlightData));
    }

    commandList->SetConstantBufferView(1, 0, *s_Data.m_PointlightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    s_Data.m_RenderStatistics.PointLightCount += s_Data.m_PointlightDrawData.size();

    for (auto& [meshHash, meshDrawData] : s_Data.m_MeshDrawData)
    {
        auto& mesh = meshDrawData.Mesh;
        auto& meshInstanceData = meshDrawData.MeshInstanceData;

        auto vertexPositionsBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_POSITION);
        auto vertexTexCoordBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_TEX_COORD);
        auto vertexNormalBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_NORMAL);
        auto indexBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_INDEX);

        commandList->SetVertexBuffers(0, 1, *vertexPositionsBuffer);
        commandList->SetVertexBuffers(1, 1, *vertexTexCoordBuffer);
        commandList->SetVertexBuffers(2, 1, *vertexNormalBuffer);
        commandList->SetIndexBuffer(*indexBuffer);

        s_Data.m_MeshInstanceBuffer->SetBufferData(&meshInstanceData[0], meshInstanceData.size() * sizeof(MeshInstanceData));
        commandList->SetVertexBuffers(3, 1, *s_Data.m_MeshInstanceBuffer);

        auto baseColorTexture = mesh->GetTexture(MeshTextureType::TEX_BASE_COLOR);
        auto normalTexture = mesh->GetTexture(MeshTextureType::TEX_NORMAL);

        commandList->SetShaderResourceView(1, 1, *baseColorTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(1, 2, *normalTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        commandList->DrawIndexed(indexBuffer->GetBufferDesc().NumElements, meshInstanceData.size());

        CD3DX12_RESOURCE_BARRIER shaderResourceBarrier[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(baseColorTexture->GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
            CD3DX12_RESOURCE_BARRIER::Transition(normalTexture->GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON)
        };
        commandList->ResourceBarrier(2, shaderResourceBarrier);

        s_Data.m_RenderStatistics.DrawCallCount++;
        s_Data.m_RenderStatistics.TriangleCount += (indexBuffer->GetBufferDesc().NumElements / 3) * meshInstanceData.size();
        s_Data.m_RenderStatistics.MeshCount += meshInstanceData.size();
    }

    RenderBackend::Get().ExecuteCommandList(commandList);

    /*
    
        Post process: Tone mapping
    
    */
    auto commandList2 = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto& tmColorTarget = s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
    auto& tmDepthBuffer = s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING]->GetDepthAttachment();

    auto tmRtv = tmColorTarget.GetDescriptorHandle();
    auto tmDsv = tmDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList2->SetViewports(1, &s_Data.m_Viewport);
    commandList2->SetScissorRects(1, &s_Data.m_ScissorRect);
    commandList2->SetRenderTargets(1, &tmRtv, &tmDsv);

    // Set pipeline state and root signature
    commandList2->SetPipelineState(s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

    // Set tone mapping vertex buffer
    commandList2->SetVertexBuffers(0, 1, *s_Data.m_ToneMapVertexBuffer.get());
    commandList2->SetIndexBuffer(*s_Data.m_ToneMapIndexBuffer.get());

    // Set shader resource view to the previous color target
    auto& defaultColorTargetTexture = s_Data.m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
    CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
    commandList2->ResourceBarrier(1, &commonBarrier);

    commandList2->SetShaderResourceView(0, 0, defaultColorTargetTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList2->DrawIndexed(s_Data.m_ToneMapIndexBuffer->GetBufferDesc().NumElements, 1);

    CD3DX12_RESOURCE_BARRIER rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList2->ResourceBarrier(1, &rtBarrier);

    RenderBackend::Get().ExecuteCommandList(commandList2);

    /*

        Debug line

    */

    if (s_Data.m_LineVertexData.size() == 0)
        return;

    auto commandList3 = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
    auto& dlColorTarget = s_Data.m_RenderPasses[RenderPassType::DEBUG_LINE]->GetColorAttachment();
    auto& dlDepthBuffer = s_Data.m_RenderPasses[RenderPassType::DEBUG_LINE]->GetDepthAttachment();

    auto dlRtv = dlColorTarget.GetDescriptorHandle();
    auto dlDsv = dlDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList3->SetViewports(1, &s_Data.m_Viewport);
    commandList3->SetScissorRects(1, &s_Data.m_ScissorRect);
    commandList3->SetRenderTargets(1, &tmRtv, &dsv);

    // Set pipeline state and root signature
    commandList3->SetPipelineState(s_Data.m_RenderPasses[RenderPassType::DEBUG_LINE]->GetPipelineState());
    commandList3->SetRootConstantBufferView(0, *s_Data.m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

    s_Data.m_LineBuffer->SetBufferData(&s_Data.m_LineVertexData[0], sizeof(LineVertex) * s_Data.m_LineVertexData.size());
    commandList3->SetVertexBuffers(0, 1, *s_Data.m_LineBuffer);

    commandList3->Draw(s_Data.m_LineVertexData.size(), 1);

    RenderBackend::Get().ExecuteCommandList(commandList3);
}

void Renderer::ImGuiRender()
{
    ImGui::Begin("Renderer");

    ImGui::Text("Resolution: %ux%u", s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", s_Data.m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Draw calls: %u", s_Data.m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", s_Data.m_RenderStatistics.TriangleCount);
    ImGui::Text("Mesh count: %u", s_Data.m_RenderStatistics.MeshCount);
    ImGui::Text("Point light count: %u", s_Data.m_RenderStatistics.PointLightCount);
    ImGui::Text("Max model instances: %u", s_Data.m_RenderSettings.MaxModelInstances);
    ImGui::Text("Max point lights: %u", s_Data.m_RenderSettings.MaxPointLights);

    ImGui::End();
}

void Renderer::EndFrame()
{
    SCOPED_TIMER("Renderer::EndFrame");

    RenderBackend::Get().ResolveToBackBuffer(s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment());
    RenderBackend::Get().SwapBuffers(s_Data.m_RenderSettings.VSync);

    s_Data.m_MeshDrawData.clear();
    s_Data.m_PointlightDrawData.clear();
    s_Data.m_LineVertexData.clear();

    s_Data.m_RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    auto iter = s_Data.m_MeshDrawData.find(mesh->GetHash());
    if (iter != s_Data.m_MeshDrawData.end())
    {
        iter->second.MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
    else
    {
        s_Data.m_MeshDrawData.emplace(std::pair<std::size_t, MeshDrawData>(mesh->GetHash(), mesh));
        s_Data.m_MeshDrawData.at(mesh->GetHash()).MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
}

void Renderer::Submit(const PointlightData& pointlightData)
{
    s_Data.m_PointlightDrawData.push_back(pointlightData);
    ASSERT((s_Data.m_PointlightDrawData.size() <= s_Data.m_RenderSettings.MaxPointLights), "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color)
{
    s_Data.m_LineVertexData.emplace_back(lineStart, color);
    s_Data.m_LineVertexData.emplace_back(lineEnd, color);
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (s_Data.m_RenderSettings.Resolution.x != width || s_Data.m_RenderSettings.Resolution.y != height)
    {
        s_Data.m_RenderSettings.Resolution.x = std::max(1u, width);
        s_Data.m_RenderSettings.Resolution.y = std::max(1u, height);

        s_Data.m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.m_RenderSettings.Resolution.x),
            static_cast<float>(s_Data.m_RenderSettings.Resolution.y), 0.0f, 1.0f);

        RenderBackend::Get().Resize(width, height);

        for (auto& renderPass : s_Data.m_RenderPasses)
            renderPass->Resize(s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
    }
}

void Renderer::ToggleVSync()
{
    s_Data.m_RenderSettings.VSync = !s_Data.m_RenderSettings.VSync;
}

bool Renderer::IsVSyncEnabled()
{
    return s_Data.m_RenderSettings.VSync;
}

const Renderer::RenderSettings& Renderer::GetSettings()
{
    return s_Data.m_RenderSettings;
}

// Temp
RenderPass& Renderer::GetRenderPass()
{
    return *s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING];
}

void Renderer::CreateRenderPasses()
{
    {
        // Default render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Default_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Default_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA16_UNORM,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        desc.DescriptorRanges.resize(2);
        desc.DescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
        desc.DescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

        desc.RootParameters.resize(2);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0);
        desc.RootParameters[1].InitAsDescriptorTable(desc.DescriptorRanges.size(), &desc.DescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.m_RenderPasses[RenderPassType::DEFAULT] = std::make_unique<RenderPass>(desc);
    }

    {
        // Debug line render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Data.m_RenderPasses[RenderPassType::DEBUG_LINE] = std::make_unique<RenderPass>(desc);
    }

    {
        // Tonemapping render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ToneMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ToneMapping_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.m_RenderSettings.Resolution.x, s_Data.m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = false;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        desc.DescriptorRanges.resize(1);
        desc.DescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsDescriptorTable(desc.DescriptorRanges.size(), &desc.DescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Data.m_RenderPasses[RenderPassType::TONE_MAPPING] = std::make_unique<RenderPass>(desc);
    }
}

void Renderer::InitializeBuffers()
{
    s_Data.m_MeshInstanceBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, s_Data.m_RenderSettings.MaxModelInstances, sizeof(MeshInstanceData)));
    s_Data.m_PointlightBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Data.m_RenderSettings.MaxPointLights, sizeof(PointlightData)));
    s_Data.m_LineBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 10000, sizeof(LineVertex)));

    // Tone mapping vertices, positions are in normalized device coordinates
    std::vector<float> toneMappingVertices = {
        -1.0f, -1.0f, 0.0f, 1.0f,
        -1.0f, 1.0f, 0.0f, 0.0f,
        1.0f, 1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 1.0f, 1.0f,
    };
    std::vector<WORD> toneMappingIndices = {
        0, 1, 2,
        2, 3, 0
    };
    s_Data.m_ToneMapVertexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, 4, sizeof(float) * 4), &toneMappingVertices[0]);
    s_Data.m_ToneMapIndexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, 6, sizeof(WORD)), &toneMappingIndices[0]);

    s_Data.m_SceneDataConstantBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
}
