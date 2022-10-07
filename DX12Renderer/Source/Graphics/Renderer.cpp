#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/Buffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Resource/Model.h"
#include "Graphics/RenderPass.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/RenderBackend.h"

#include <imgui/imgui.h>

struct SceneData
{
    SceneData() = default;

    glm::mat4 ViewProjection = glm::identity<glm::mat4>();
    glm::vec3 Ambient = glm::vec3(0.0f);

    uint32_t NumDirLights = 0;
    uint32_t NumPointLights = 0;
    uint32_t NumSpotLights = 0;
};

struct RendererStatistics
{
    void Reset()
    {
        DrawCallCount = 0;
        TriangleCount = 0;
        MeshCount = 0;
        DirectionalLightCount = 0;
        PointLightCount = 0;
        SpotLightCount = 0;
    }

    uint32_t DrawCallCount = 0;
    uint32_t TriangleCount = 0;
    uint32_t MeshCount = 0;
    uint32_t DirectionalLightCount = 0;
    uint32_t PointLightCount = 0;
    uint32_t SpotLightCount = 0;
};

enum RenderPassType : uint32_t
{
    SHADOW_MAPPING,
    LIGHTING,
    TONE_MAPPING,
    NUM_RENDER_PASSES = (TONE_MAPPING + 1)
};

struct MeshInstanceData
{
    MeshInstanceData(const glm::mat4& transform, const glm::vec4& color, uint32_t baseColorTexIndex, uint32_t normalTexIndex)
        : Transform(transform), Color(color), BaseColorTexIndex(baseColorTexIndex), NormalTexIndex(normalTexIndex) {}

    glm::mat4 Transform = glm::identity<glm::mat4>();
    glm::vec4 Color = glm::vec4(1.0f);
    uint32_t BaseColorTexIndex = 0;
    uint32_t NormalTexIndex = 0;
};

struct MeshDrawData
{
    MeshDrawData(const std::shared_ptr<Mesh>& mesh)
        : Mesh(mesh) {}

    std::shared_ptr<Mesh> Mesh;
    std::vector<MeshInstanceData> MeshInstanceData;
};

enum class TonemapType : uint32_t
{
    LINEAR, REINHARD, UNCHARTED2, FILMIC, ACES_FILMIC,
    NUM_TYPES = 5
};

struct TonemapSettings
{
    uint32_t HDRTargetIndex = 0;
    float Exposure = 1.5f;
    float Gamma = 2.2f;
    TonemapType Type = TonemapType::REINHARD;
};

std::string TonemapTypeToString(TonemapType type)
{
    switch (type)
    {
    case TonemapType::LINEAR:
        return std::string("Linear");
    case TonemapType::REINHARD:
        return std::string("Reinhard");
    case TonemapType::UNCHARTED2:
        return std::string("Uncharted2");
    case TonemapType::FILMIC:
        return std::string("Filmic");
    case TonemapType::ACES_FILMIC:
        return std::string("ACES Filmic");
    default:
        return std::string("Unknown");
    }
}

struct InternalRendererData
{
    // Render passes
    std::unique_ptr<RenderPass> RenderPasses[RenderPassType::NUM_RENDER_PASSES];

    // Renderer settings and statistics
    Renderer::RenderSettings RenderSettings;
    RendererStatistics RenderStatistics;

    // Tone mapping settings and buffers
    TonemapSettings TonemapSettings;
    std::unique_ptr<Buffer> TonemapConstantBuffer;
    std::unique_ptr<Buffer> TonemapVertexBuffer;
    std::unique_ptr<Buffer> TonemapIndexBuffer;

    // Scene data and buffer
    SceneData SceneData;
    std::unique_ptr<Buffer> SceneDataConstantBuffer;

    // Mesh draw data and buffer
    std::unordered_map<std::size_t, MeshDrawData> MeshDrawData;
    std::unique_ptr<Buffer> MeshInstanceBuffer;

    // Directional light data and buffer
    std::vector<DirectionalLightData> DirectionalLightData;
    std::vector<std::shared_ptr<Texture>> ShadowMaps;
    std::unique_ptr<Buffer> DirectionalLightConstantBuffer;

    // Pointlight data and buffer
    std::vector<PointLightData> PointLightData;
    std::unique_ptr<Buffer> PointLightConstantBuffer;

    // Spotlight data and buffer
    std::vector<SpotLightData> SpotLightData;
    std::unique_ptr<Buffer> SpotLightConstantBuffer;
};

static InternalRendererData s_Data;

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    RenderBackend::Initialize(hWnd, width, height);

    s_Data.RenderSettings.Resolution.x = width;
    s_Data.RenderSettings.Resolution.y = height;

    MakeRenderPasses();
    MakeBuffers();
}

void Renderer::Finalize()
{
    RenderBackend::Finalize();
}

void Renderer::BeginScene(const Camera& sceneCamera, const glm::vec3& ambient)
{
    SCOPED_TIMER("Renderer::BeginScene");
    
    s_Data.SceneData.ViewProjection = sceneCamera.GetViewProjection();
    s_Data.SceneData.Ambient = ambient;

    s_Data.TonemapSettings.Exposure = sceneCamera.GetExposure();
    s_Data.TonemapSettings.Gamma = sceneCamera.GetGamma();

    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (auto& renderPass : s_Data.RenderPasses)
        renderPass->ClearViews(commandList);

    RenderBackend::ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    PrepareInstanceBuffer();
    PrepareLightBuffers();
    PrepareShadowMaps();

    auto& descriptorHeap = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.Resolution.x), static_cast<float>(s_Data.RenderSettings.Resolution.y), 0.0f, 1.0f);
    CD3DX12_RECT scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    {
        /* Shadow mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        CD3DX12_VIEWPORT shadowmapViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.ShadowMapSize), static_cast<float>(s_Data.RenderSettings.ShadowMapSize), 0.0f, 1.0f);
        commandList->SetViewports(1, &shadowmapViewport);
        commandList->SetScissorRects(1, &scissorRect);

        for (uint32_t lightIndex = 0; lightIndex < s_Data.DirectionalLightData.size(); ++lightIndex)
        {
            GenerateShadowMap(*commandList.get(), s_Data.DirectionalLightData[lightIndex].ViewProjection, *s_Data.ShadowMaps[lightIndex].get());
        }

        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Lighting render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        auto& lightingColorTarget = s_Data.RenderPasses[RenderPassType::LIGHTING]->GetColorAttachment();
        auto& lightingDepthBuffer = s_Data.RenderPasses[RenderPassType::LIGHTING]->GetDepthAttachment();

        auto rtv = lightingColorTarget.GetDescriptorHandle(DescriptorType::RTV);
        auto dsv = lightingDepthBuffer.GetDescriptorHandle(DescriptorType::DSV);

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &viewport);
        commandList->SetScissorRects(1, &scissorRect);
        commandList->SetRenderTargets(1, &rtv, &dsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set the pipeline state along with the root signature
        commandList->SetPipelineState(s_Data.RenderPasses[RenderPassType::LIGHTING]->GetPipelineState());

        // Set root CBVs for scene data
        s_Data.SceneDataConstantBuffer->SetBufferData(&s_Data.SceneData);
        commandList->SetRootConstantBufferView(0, *s_Data.SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set root CBVs for lights
        commandList->SetRootConstantBufferView(1, *s_Data.DirectionalLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        commandList->SetRootConstantBufferView(2, *s_Data.PointLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        commandList->SetRootConstantBufferView(3, *s_Data.SpotLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(4, descriptorHeap.GetGPUBaseDescriptor());

        // Set instance buffer
        commandList->SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer);
        uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * s_Data.RenderSettings.MaxInstancesPerDraw;

        std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

        for (auto& [meshHash, meshDrawData] : s_Data.MeshDrawData)
        {
            auto& mesh = meshDrawData.Mesh;
            auto& meshInstanceData = meshDrawData.MeshInstanceData;

            auto vb = mesh->GetVertexBuffer();
            auto ib = mesh->GetIndexBuffer();

            if (mesh->GetVertexBuffer() != currentVertexBuffer)
            {
                commandList->SetVertexBuffers(0, 1, *vb);
                currentVertexBuffer = vb;
            }
            if (mesh->GetIndexBuffer() != currentIndexBuffer)
            {
                commandList->SetIndexBuffer(*ib);
                currentIndexBuffer = ib;
            }

            commandList->DrawIndexed(mesh->GetNumIndices(), meshInstanceData.size(), mesh->GetStartIndex(), mesh->GetStartVertex(), startInstance);
            startInstance += meshInstanceData.size();

            s_Data.RenderStatistics.DrawCallCount++;
            s_Data.RenderStatistics.TriangleCount += (mesh->GetNumIndices() / 3) * meshInstanceData.size();
            s_Data.RenderStatistics.MeshCount += meshInstanceData.size();
        }

        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Tone mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        auto& tmColorTarget = s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
        auto& tmDepthBuffer = s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetDepthAttachment();

        auto tmRtv = tmColorTarget.GetDescriptorHandle(DescriptorType::RTV);
        auto tmDsv = tmDepthBuffer.GetDescriptorHandle(DescriptorType::DSV);

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &viewport);
        commandList->SetScissorRects(1, &scissorRect);
        commandList->SetRenderTargets(1, &tmRtv, &tmDsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set pipeline state and root signature
        commandList->SetPipelineState(s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

        uint32_t pbrColorTargetIndex = s_Data.RenderPasses[RenderPassType::LIGHTING]->GetColorAttachment().GetDescriptorIndex(DescriptorType::SRV);
        s_Data.TonemapSettings.HDRTargetIndex = pbrColorTargetIndex;
        s_Data.TonemapConstantBuffer->SetBufferData(&s_Data.TonemapSettings);
        commandList->SetRootConstantBufferView(0, *s_Data.TonemapConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(1, descriptorHeap.GetGPUBaseDescriptor());

        // Set tone mapping vertex buffer
        commandList->SetVertexBuffers(0, 1, *s_Data.TonemapVertexBuffer.get());
        commandList->SetIndexBuffer(*s_Data.TonemapIndexBuffer.get());

        commandList->DrawIndexed(s_Data.TonemapIndexBuffer->GetBufferDesc().NumElements, 1);

        RenderBackend::ExecuteCommandList(commandList);
    }
}

void Renderer::OnImGuiRender()
{
    ImGui::Text("Renderer");
    ImGui::Text("Resolution: %ux%u", s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", s_Data.RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Shadow map resolution: %u", s_Data.RenderSettings.ShadowMapSize);
    ImGui::Text("Max model instances: %u", s_Data.RenderSettings.MaxModelInstances);
    ImGui::Text("Max instances per draw: %u", s_Data.RenderSettings.MaxInstancesPerDraw);
    ImGui::Text("Max directional lights: %u", s_Data.RenderSettings.MaxDirectionalLights);
    ImGui::Text("Max point lights: %u", s_Data.RenderSettings.MaxPointLights);
    ImGui::Text("Max spot lights: %u", s_Data.RenderSettings.MaxSpotLights);

    if (ImGui::CollapsingHeader("Stats"))
    {
        ImGui::Text("Draw calls: %u", s_Data.RenderStatistics.DrawCallCount);
        ImGui::Text("Triangle count: %u", s_Data.RenderStatistics.TriangleCount);
        ImGui::Text("Mesh count: %u", s_Data.RenderStatistics.MeshCount);
        ImGui::Text("Directional light count: %u", s_Data.RenderStatistics.DirectionalLightCount);
        ImGui::Text("Point light count: %u", s_Data.RenderStatistics.PointLightCount);
        ImGui::Text("Spot light count: %u", s_Data.RenderStatistics.SpotLightCount);
    }

    if (ImGui::CollapsingHeader("Tonemapping"))
    {
        ImGui::Text("Tonemapping type");
        std::string previewValue = TonemapTypeToString(s_Data.TonemapSettings.Type);
        if (ImGui::BeginCombo("##TonemapType", previewValue.c_str()))
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(TonemapType::NUM_TYPES); ++i)
            {
                std::string tonemapName = TonemapTypeToString(static_cast<TonemapType>(i)).c_str();
                bool isSelected = previewValue == tonemapName;

                if (ImGui::Selectable(tonemapName.c_str(), isSelected))
                    s_Data.TonemapSettings.Type = static_cast<TonemapType>(i);
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }
    }
}

void Renderer::EndScene()
{
    SCOPED_TIMER("Renderer::EndScene");

    RenderBackend::GetSwapChain().ResolveToBackBuffer(s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment());
    RenderBackend::GetSwapChain().SwapBuffers(s_Data.RenderSettings.VSync);

    s_Data.MeshDrawData.clear();

    s_Data.ShadowMaps.clear();
    s_Data.DirectionalLightData.clear();
    s_Data.PointLightData.clear();
    s_Data.SpotLightData.clear();

    s_Data.RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    uint32_t baseColorTexIndex = mesh->GetTexture(MeshTextureType::TEX_BASE_COLOR)->GetDescriptorIndex(DescriptorType::SRV);
    uint32_t normalTexIndex = mesh->GetTexture(MeshTextureType::TEX_NORMAL)->GetDescriptorIndex(DescriptorType::SRV);

    auto iter = s_Data.MeshDrawData.find(mesh->GetHash());

    if (iter != s_Data.MeshDrawData.end())
    {
        iter->second.MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f),
            baseColorTexIndex, normalTexIndex));
    }
    else
    {
        s_Data.MeshDrawData.emplace(std::pair<std::size_t, MeshDrawData>(mesh->GetHash(), mesh));
        s_Data.MeshDrawData.at(mesh->GetHash()).MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f),
            baseColorTexIndex, normalTexIndex));
    }
}

void Renderer::Submit(const DirectionalLightData& dirLightData, const std::shared_ptr<Texture> shadowMap)
{
    s_Data.DirectionalLightData.push_back(dirLightData);
    s_Data.ShadowMaps.push_back(shadowMap);

    ASSERT(s_Data.DirectionalLightData.size() <= s_Data.RenderSettings.MaxDirectionalLights, "Exceeded the maximum amount of directional lights");
}

void Renderer::Submit(const PointLightData& pointlightData, const std::shared_ptr<Texture> shadowMap)
{
    s_Data.PointLightData.push_back(pointlightData);
    s_Data.ShadowMaps.push_back(shadowMap);

    ASSERT(s_Data.PointLightData.size() <= s_Data.RenderSettings.MaxPointLights, "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const SpotLightData& spotlightData, const std::shared_ptr<Texture> shadowMap)
{
    s_Data.SpotLightData.push_back(spotlightData);
    s_Data.ShadowMaps.push_back(shadowMap);

    ASSERT(s_Data.SpotLightData.size() <= s_Data.RenderSettings.MaxSpotLights, "Exceeded the maximum amount of spot lights");
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (s_Data.RenderSettings.Resolution.x != width || s_Data.RenderSettings.Resolution.y != height)
    {
        s_Data.RenderSettings.Resolution.x = std::max(1u, width);
        s_Data.RenderSettings.Resolution.y = std::max(1u, height);

        RenderBackend::Resize(width, height);

        for (auto& renderPass : s_Data.RenderPasses)
            renderPass->Resize(s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
    }
}

void Renderer::ToggleVSync()
{
    s_Data.RenderSettings.VSync = !s_Data.RenderSettings.VSync;
}

bool Renderer::IsVSyncEnabled()
{
    return s_Data.RenderSettings.VSync;
}

const Renderer::RenderSettings& Renderer::GetSettings()
{
    return s_Data.RenderSettings;
}

// Temp
Texture& Renderer::GetFinalColorOutput()
{
    return s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
}

// Temp
Texture& Renderer::GetFinalDepthOutput()
{
    return s_Data.RenderPasses[RenderPassType::LIGHTING]->GetDepthAttachment();
}

void Renderer::MakeRenderPasses()
{
    {
        // Shadow mapping render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ShadowMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ShadowMapping_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_UNSPECIFIED,
            0, 0);
        // Resolution of shadow maps should be adjustable at runtime
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            0, 0);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstants(16, 0); // Light VP

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "TEX_INDICES", 0, DXGI_FORMAT_R32G32_UINT, 1, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING] = std::make_unique<RenderPass>("Shadow mapping", desc);
    }

    {
        // Lighting render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Lighting_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Lighting_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
            s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        desc.RootParameters.resize(5);
        desc.RootParameters[0].InitAsConstantBufferView(0); // Scene data
        desc.RootParameters[1].InitAsConstantBufferView(1); // Directional lights
        desc.RootParameters[2].InitAsConstantBufferView(2); // Pointlights
        desc.RootParameters[3].InitAsConstantBufferView(3); // Spotlights
        desc.RootParameters[4].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Bindless SRV table

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "TEX_INDICES", 0, DXGI_FORMAT_R32G32_UINT, 1, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::LIGHTING] = std::make_unique<RenderPass>("Lighting", desc);
    }

    {
        // Tonemapping render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ToneMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ToneMapping_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.RenderSettings.Resolution.x, s_Data.RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = false;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        desc.RootParameters.resize(2);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL); // Tonemapping settings
        desc.RootParameters[1].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Bindless SRV table

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Data.RenderPasses[RenderPassType::TONE_MAPPING] = std::make_unique<RenderPass>("Tonemapping", desc);
    }
}

void Renderer::MakeBuffers()
{
    s_Data.MeshInstanceBuffer = std::make_unique<Buffer>("Mesh instance buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD,
        s_Data.RenderSettings.MaxInstancesPerDraw * RenderBackend::GetSwapChain().GetBackBufferCount(), sizeof(MeshInstanceData)));

    s_Data.DirectionalLightConstantBuffer = std::make_unique<Buffer>("Directional lights constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Data.RenderSettings.MaxDirectionalLights, sizeof(DirectionalLightData)));
    s_Data.PointLightConstantBuffer = std::make_unique<Buffer>("Pointlights constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Data.RenderSettings.MaxPointLights, sizeof(PointLightData)));
    s_Data.SpotLightConstantBuffer = std::make_unique<Buffer>("Spotlights constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Data.RenderSettings.MaxSpotLights, sizeof(SpotLightData)));

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
    s_Data.TonemapConstantBuffer = std::make_unique<Buffer>("Tonemap constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(TonemapSettings)));
    s_Data.TonemapVertexBuffer = std::make_unique<Buffer>("Tonemap vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, 4, sizeof(float) * 4), &toneMappingVertices[0]);
    s_Data.TonemapIndexBuffer = std::make_unique<Buffer>("Tonemap index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, 6, sizeof(WORD)), &toneMappingIndices[0]);

    s_Data.SceneDataConstantBuffer = std::make_unique<Buffer>("Scene data constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
}

void Renderer::PrepareInstanceBuffer()
{
    uint32_t currentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();
    std::size_t currentByteOffset = static_cast<std::size_t>(currentBackBufferIndex * s_Data.RenderSettings.MaxInstancesPerDraw) * sizeof(MeshInstanceData);

    for (auto& [meshHash, meshDrawData] : s_Data.MeshDrawData)
    {
        std::size_t numBytes = meshDrawData.MeshInstanceData.size() * sizeof(MeshInstanceData);
        s_Data.MeshInstanceBuffer->SetBufferDataAtOffset(&meshDrawData.MeshInstanceData[0], numBytes, currentByteOffset);
        currentByteOffset += numBytes;
    }
}

void Renderer::PrepareLightBuffers()
{
    // Update scene data constant buffer with data for this frame
    s_Data.SceneData.NumDirLights = s_Data.DirectionalLightData.size();
    s_Data.SceneData.NumPointLights = s_Data.PointLightData.size();
    s_Data.SceneData.NumSpotLights = s_Data.SpotLightData.size();

    // Set constant buffer data for directional lights/pointlights/spotlights
    if (s_Data.SceneData.NumDirLights > 0)
        s_Data.DirectionalLightConstantBuffer->SetBufferData(&s_Data.DirectionalLightData[0], s_Data.DirectionalLightData.size() * sizeof(DirectionalLightData));
    s_Data.RenderStatistics.DirectionalLightCount += s_Data.DirectionalLightData.size();

    if (s_Data.SceneData.NumPointLights > 0)
        s_Data.PointLightConstantBuffer->SetBufferData(&s_Data.PointLightData[0], s_Data.PointLightData.size() * sizeof(PointLightData));
    s_Data.RenderStatistics.PointLightCount += s_Data.PointLightData.size();

    if (s_Data.SceneData.NumSpotLights > 0)
        s_Data.SpotLightConstantBuffer->SetBufferData(&s_Data.SpotLightData[0], s_Data.SpotLightData.size() * sizeof(SpotLightData));
    s_Data.RenderStatistics.SpotLightCount += s_Data.SpotLightData.size();
}

void Renderer::PrepareShadowMaps()
{
    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (auto& shadowMap : s_Data.ShadowMaps)
    {
        commandList->ClearDepthStencilView(shadowMap->GetDescriptorHandle(DescriptorType::DSV));
    }

    RenderBackend::ExecuteCommandListAndWait(commandList);
}

void Renderer::GenerateShadowMap(CommandList& commandList, const glm::mat4& lightVP, const Texture& shadowMap)
{
    auto dsv = shadowMap.GetDescriptorHandle(DescriptorType::DSV);
    commandList.SetRenderTargets(0, nullptr, &dsv);

    // Set the pipeline state along with the root signature
    commandList.SetPipelineState(s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING]->GetPipelineState());

    // Set root constant (light VP)
    commandList.SetRootConstants(0, 16, &lightVP[0][0], 0);

    // Set instance buffer
    commandList.SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer);
    uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * s_Data.RenderSettings.MaxInstancesPerDraw;

    std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

    for (auto& [meshHash, meshDrawData] : s_Data.MeshDrawData)
    {
        auto& mesh = meshDrawData.Mesh;
        auto& meshInstanceData = meshDrawData.MeshInstanceData;

        auto vb = mesh->GetVertexBuffer();
        auto ib = mesh->GetIndexBuffer();

        if (mesh->GetVertexBuffer() != currentVertexBuffer)
        {
            commandList.SetVertexBuffers(0, 1, *vb);
            currentVertexBuffer = vb;
        }
        if (mesh->GetIndexBuffer() != currentIndexBuffer)
        {
            commandList.SetIndexBuffer(*ib);
            currentIndexBuffer = ib;
        }

        commandList.DrawIndexed(mesh->GetNumIndices(), meshInstanceData.size(), mesh->GetStartIndex(), mesh->GetStartVertex(), startInstance);
        startInstance += meshInstanceData.size();
    }
}
