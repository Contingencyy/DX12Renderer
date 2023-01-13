#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Resource/Model.h"
#include "Graphics/RasterPass.h"
#include "Graphics/ComputePass.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/RenderBackend.h"

#include <imgui/imgui.h>

constexpr uint32_t MAX_MESH_INSTANCES = 1000;
constexpr uint32_t MAX_MESHES = 1000;
constexpr uint32_t MAX_MATERIALS = 1000;
constexpr uint32_t MAX_DIR_LIGHTS = 1;
constexpr uint32_t MAX_POINT_LIGHTS = 50;
constexpr uint32_t MAX_SPOT_LIGHTS = 50;

struct SceneData
{
    void Reset()
    {
        NumDirLights = 0;
        NumPointLights = 0;
        NumSpotLights = 0;
    }

    glm::mat4 SceneCameraViewProjection = glm::identity<glm::mat4>();
    glm::vec3 SceneCameraPosition = glm::vec3(0.0f);

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
    }

    uint32_t DrawCallCount = 0;
    uint32_t TriangleCount = 0;
    uint32_t MeshCount = 0;
};

enum RenderPassType : uint32_t
{
    SHADOW_MAPPING,
    DEPTH_PREPASS,
    LIGHTING,
    NUM_RENDER_PASSES
};

enum ComputePassType : uint32_t
{
    TONE_MAPPING,
    NUM_COMPUTE_PASSES
};

struct MaterialData
{
    MaterialData() = default;

    uint32_t BaseColorTextureIndex = 0;
    uint32_t NormalTextureIndex = 0;
    uint32_t MetallicRoughnessTextureIndex = 0;
    float Metalness = 0.0f;
    float Roughness = 0.3f;
    BYTE_PADDING(12);
};;

struct MeshInstanceData
{
    MeshInstanceData() = default;

    glm::mat4 Transform = glm::identity<glm::mat4>();
    uint32_t MaterialID = 0;
};

struct MeshDrawData
{
    MeshPrimitive Primitive;
    MeshInstanceData InstanceData;
};

enum class TonemapType : uint32_t
{
    LINEAR, REINHARD, UNCHARTED2, FILMIC, ACES_FILMIC,
    NUM_TYPES = 5
};

struct TonemapSettings
{
    uint32_t HDRRenderTargetIndex = 0;
    uint32_t SDRRenderTargetIndex = 0;
    float Exposure = 1.5f;
    float Gamma = 2.2f;
    TonemapType Type = TonemapType::UNCHARTED2;
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
    std::unique_ptr<RasterPass> RenderPasses[RenderPassType::NUM_RENDER_PASSES];
    std::unique_ptr<ComputePass> ComputePasses[ComputePassType::NUM_COMPUTE_PASSES];
    std::array<std::shared_ptr<FrameBuffer>, RenderPassType::NUM_RENDER_PASSES> FrameBuffers;
    std::unique_ptr<Texture> SDRRenderTarget;

    // Renderer settings and statistics
    Renderer::RenderSettings RenderSettings;
    RendererStatistics RenderStatistics;

    // Tone mapping settings and buffers
    TonemapSettings TonemapSettings;
    std::unique_ptr<Buffer> TonemapConstantBuffer;

    // Scene data and buffer
    Camera SceneCamera;
    SceneData SceneData;
    std::unique_ptr<Buffer> SceneDataConstantBuffer;

    // Mesh draw data and buffer
    std::array<std::unique_ptr<Buffer>, TransparencyMode::NUM_ALPHA_MODES> MeshInstanceBuffer;
    std::array<std::array<MeshDrawData, 1000>, TransparencyMode::NUM_ALPHA_MODES> MeshDrawData;
    std::size_t NumMeshes[TransparencyMode::NUM_ALPHA_MODES];

    // Material constant buffer
    std::unique_ptr<Buffer> MaterialConstantBuffer;
    std::size_t NumMaterials;

    // Directional/point/spotlight constant buffers
    std::unique_ptr<Buffer> LightConstantBuffer;

    // Directional/point/spotlight draw data
    std::array<DirectionalLightData, MAX_DIR_LIGHTS> DirectionalLightDrawData;
    std::array<PointLightData, MAX_POINT_LIGHTS> PointLightDrawData;
    std::array<SpotLightData, MAX_SPOT_LIGHTS> SpotLightDrawData;

    // Light cameras and light shadow maps
    std::array<Camera, MAX_DIR_LIGHTS + 6 * MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS> LightCameras;
    std::array<std::shared_ptr<Texture>, MAX_DIR_LIGHTS + MAX_POINT_LIGHTS + MAX_SPOT_LIGHTS> LightShadowMaps;
    
    uint32_t CurrentBackBufferIndex = 0;

    std::unique_ptr<Texture> WhiteTexture;
    std::unique_ptr<Texture> DefaultNormalTexture;
};

static InternalRendererData s_Data;

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    RenderBackend::Initialize(hWnd, width, height);

    s_Data.RenderSettings.RenderResolution.x = width;
    s_Data.RenderSettings.RenderResolution.y = height;

    MakeRenderPasses();
    MakeBuffers();
    MakeFrameBuffers();

    // Make default white and normal textures
    uint32_t whiteTextureData = 0xFFFFFFFF;
    s_Data.WhiteTexture = std::make_unique<Texture>("Default white texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
        TextureDimension::TEXTURE_DIMENSION_2D, 1, 1), &whiteTextureData);

    // Texture data is in ABGR layout, the default normal will point forward in tangent space (blue)
    uint32_t defaultNormalTextureData = (255 << 24) + (255 << 16) + (0 << 8) + 0;
    s_Data.DefaultNormalTexture = std::make_unique<Texture>("Default normal texture", TextureDesc(TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
        TextureDimension::TEXTURE_DIMENSION_2D, 1, 1), &defaultNormalTextureData);
}

void Renderer::Finalize()
{
    RenderBackend::Finalize();
}

void Renderer::BeginScene(const Camera& sceneCamera)
{
    SCOPED_TIMER("Renderer::BeginScene");

    s_Data.CurrentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();
    
    s_Data.SceneCamera = sceneCamera;
    s_Data.SceneData.SceneCameraViewProjection = sceneCamera.GetViewProjection();
    s_Data.SceneData.SceneCameraPosition = sceneCamera.GetTransform().GetPosition();

    s_Data.TonemapSettings.Exposure = sceneCamera.GetExposure();
    s_Data.TonemapSettings.Gamma = sceneCamera.GetGamma();

    for (auto& frameBuffer : s_Data.FrameBuffers)
    {
        frameBuffer->ClearAttachments();
    }
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    for (uint32_t i = 0; i < TransparencyMode::NUM_ALPHA_MODES; ++i)
    {
        s_Data.RenderStatistics.MeshCount = s_Data.NumMeshes[i];

        for (uint32_t j = 0; j < s_Data.NumMeshes[i]; ++j)
            s_Data.RenderStatistics.TriangleCount += s_Data.MeshDrawData[i][j].Primitive.NumIndices / 3;
    }

    PrepareInstanceBuffer();
    PrepareLightBuffers();

    auto& descriptorHeap = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    s_Data.SceneDataConstantBuffer->SetBufferData(&s_Data.SceneData);

    {
        /* Shadow mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Shadow mapping");

        // Clear all shadow map textures and transition them to the depth write state
        for (auto& shadowMap : s_Data.LightShadowMaps)
        {
            if (shadowMap)
            {
                commandList->Transition(*shadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE);
                commandList->ClearDepthStencilView(shadowMap->GetDescriptor(DescriptorType::DSV), 0.0f);
            }
        }

        // Set bindless descriptor heap
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Bind render pass bindables (sets viewport, scissor rect, render targets, pipeline state, root signature and primitive topology)
        commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING]);

        std::size_t cameraIndex = 0, shadowMapIndex = 0;
        for (uint32_t i = 0; i < s_Data.SceneData.NumDirLights; ++i)
        {
            RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], *s_Data.LightShadowMaps[shadowMapIndex]);

            cameraIndex++;
            shadowMapIndex++;
        }

        cameraIndex = MAX_DIR_LIGHTS;
        shadowMapIndex = MAX_DIR_LIGHTS;
        for (uint32_t i = 0; i < s_Data.SceneData.NumPointLights; ++i)
        {
            for (uint32_t face = 0; face < 6; ++face)
            {
                RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], *s_Data.LightShadowMaps[shadowMapIndex], 1 + face);
                cameraIndex++;
            }

            shadowMapIndex++;
        }

        cameraIndex = static_cast<std::size_t>(MAX_DIR_LIGHTS + (MAX_POINT_LIGHTS * 6));
        shadowMapIndex = static_cast<std::size_t>(MAX_DIR_LIGHTS + MAX_POINT_LIGHTS);
        for (uint32_t i = 0; i < s_Data.SceneData.NumSpotLights; ++i)
        {
            RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], *s_Data.LightShadowMaps[shadowMapIndex]);

            cameraIndex++;
            shadowMapIndex++;
        }

        commandList->EndTimestampQuery("Shadow mapping");
        RenderBackend::ExecuteCommandList(commandList);
    }

    const std::string timestampNames[4] = { "Depth pre-pass (opaque)", "Depth pre-pass (transparent)", "Lighting (opaque)", "Lighting (transparent)" };

    for (uint32_t i = 0; i < TransparencyMode::NUM_ALPHA_MODES; ++i)
    {
        {
            /* Depth pre-pass render pass */
            auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            commandList->BeginTimestampQuery(timestampNames[i]);

            auto& depthTarget = s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS]->GetDepthAttachment();
            commandList->Transition(depthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            // Bind render pass bindables
            commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::DEPTH_PREPASS]);

            CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.RenderResolution.x),
                static_cast<float>(s_Data.RenderSettings.RenderResolution.y), 0.0f, 1.0f);
            CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

            commandList->SetViewports(1, &viewport);
            commandList->SetScissorRects(1, &scissorRect);

            D3D12_CPU_DESCRIPTOR_HANDLE dsv = depthTarget.GetDescriptor(DescriptorType::DSV);
            commandList->SetRenderTargets(0, nullptr, &dsv);

            commandList->SetRootConstantBufferView(0, *s_Data.SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

            RenderGeometry(*commandList, s_Data.SceneCamera, i);

            commandList->EndTimestampQuery(timestampNames[i]);
            RenderBackend::ExecuteCommandList(commandList);
        }

        {
            /* Lighting render pass */
            auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            commandList->BeginTimestampQuery(timestampNames[2 + i]);

            // Transition all shadow maps to the pixel shader resource state
            for (auto& shadowMap : s_Data.LightShadowMaps)
            {
                if (shadowMap)
                    commandList->Transition(*shadowMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }

            // Set bindless descriptor heap
            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

            // Bind render pass bindables (sets viewport, scissor rect, render targets, pipeline state, root signature and primitive topology)
            commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::LIGHTING]);

            CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.RenderResolution.x),
                static_cast<float>(s_Data.RenderSettings.RenderResolution.y), 0.0f, 1.0f);
            CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

            commandList->SetViewports(1, &viewport);
            commandList->SetScissorRects(1, &scissorRect);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = s_Data.FrameBuffers[RenderPassType::LIGHTING]->GetColorAttachment().GetDescriptor(DescriptorType::RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS]->GetDepthAttachment().GetDescriptor(DescriptorType::DSV);
            commandList->SetRenderTargets(1, &rtv, &dsv);

            // Set root CBV for scene data constant buffer
            commandList->SetRootConstantBufferView(0, *s_Data.SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);
            // Set root CBV for material constant buffer
            commandList->SetRootConstantBufferView(1, *s_Data.MaterialConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);
            // Set root CBV for light constant buffer
            commandList->SetRootConstantBufferView(2, *s_Data.LightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
            // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
            commandList->SetRootDescriptorTable(3, descriptorHeap.GetGPUBaseDescriptor());

            RenderGeometry(*commandList, s_Data.SceneCamera, i);

            commandList->EndTimestampQuery(timestampNames[2 + i]);
            RenderBackend::ExecuteCommandList(commandList);
        }
    }

    {
        /* Tone mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Tonemapping");

        // Transition the HDR render target to pixel shader resource state
        auto& hdrColorTarget = s_Data.FrameBuffers[RenderPassType::LIGHTING]->GetColorAttachment();
        commandList->Transition(hdrColorTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->Transition(*s_Data.SDRRenderTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->GetGraphicsCommandList()->SetComputeRootSignature(s_Data.ComputePasses[ComputePassType::TONE_MAPPING]->GetD3D12RootSignature().Get());
        commandList->GetGraphicsCommandList()->SetPipelineState(s_Data.ComputePasses[ComputePassType::TONE_MAPPING]->GetD3D12PipelineState().Get());

        ID3D12DescriptorHeap* const heaps = { RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetD3D12DescriptorHeap().Get() };
        commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &heaps);

        commandList->GetGraphicsCommandList()->SetComputeRootConstantBufferView(0, s_Data.TonemapConstantBuffer->GetD3D12Resource()->GetGPUVirtualAddress());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(1, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(2, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());

        uint32_t threadX = MathHelper::AlignUp(s_Data.RenderSettings.RenderResolution.x, 8) / 8;
        uint32_t threadY = MathHelper::AlignUp(s_Data.RenderSettings.RenderResolution.y, 8) / 8;
        commandList->GetGraphicsCommandList()->Dispatch(threadX, threadY, 1);

        commandList->EndTimestampQuery("Tonemapping");
        RenderBackend::ExecuteCommandList(commandList);
    }
}

void Renderer::OnImGuiRender()
{
    ImGui::Text("Renderer");
    ImGui::Text("Render resolution: %ux%u", s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    ImGui::Text("VSync: %s", s_Data.RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Shadow map resolution: %ux%u", s_Data.RenderSettings.ShadowMapResolution, s_Data.RenderSettings.ShadowMapResolution);

    if (ImGui::CollapsingHeader("Stats"))
    {
        ImGui::Text("Draw calls: %u", s_Data.RenderStatistics.DrawCallCount);
        ImGui::Text("Triangle count: %u", s_Data.RenderStatistics.TriangleCount);
        ImGui::Text("Mesh count: %u", s_Data.RenderStatistics.MeshCount);
        ImGui::Text("Directional light count: %u", s_Data.SceneData.NumDirLights);
        ImGui::Text("Point light count: %u", s_Data.SceneData.NumPointLights);
        ImGui::Text("Spot light count: %u", s_Data.SceneData.NumSpotLights);
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

    RenderBackend::GetSwapChain().ResolveToBackBuffer(*s_Data.SDRRenderTarget);

    s_Data.NumMeshes[TransparencyMode::OPAQUE] = 0;
    s_Data.NumMeshes[TransparencyMode::TRANSPARENT] = 0;
    s_Data.NumMaterials = 0;

    s_Data.SceneData.Reset();
    s_Data.RenderStatistics.Reset();
}

void Renderer::Submit(const MeshPrimitive& meshPrimitive, const BoundingBox& bb, const glm::mat4& transform)
{
    uint32_t albedoTextureIndex = s_Data.WhiteTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    if (meshPrimitive.Material.AlbedoTexture->IsValid())
        albedoTextureIndex = meshPrimitive.Material.AlbedoTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    uint32_t normalTextureIndex = s_Data.DefaultNormalTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    if (meshPrimitive.Material.NormalTexture->IsValid())
        normalTextureIndex = meshPrimitive.Material.NormalTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    uint32_t metallicRoughnessTextureIndex = s_Data.WhiteTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    if (meshPrimitive.Material.MetallicRoughnessTexture->IsValid())
        metallicRoughnessTextureIndex = meshPrimitive.Material.MetallicRoughnessTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    auto& meshDrawData = s_Data.MeshDrawData[meshPrimitive.Material.Transparency];
    std::size_t& numMeshes = s_Data.NumMeshes[meshPrimitive.Material.Transparency];

    MaterialData materialData = {};
    materialData.BaseColorTextureIndex = albedoTextureIndex;
    materialData.NormalTextureIndex = normalTextureIndex;
    materialData.MetallicRoughnessTextureIndex = metallicRoughnessTextureIndex;
    materialData.Metalness = meshPrimitive.Material.MetalnessFactor;
    materialData.Roughness = meshPrimitive.Material.RoughnessFactor;
    s_Data.MaterialConstantBuffer->SetBufferDataAtOffset(&materialData, sizeof(MaterialData), s_Data.NumMaterials * sizeof(MaterialData));

    meshDrawData[numMeshes].Primitive = meshPrimitive;
    meshDrawData[numMeshes].Primitive.BB = bb;
    meshDrawData[numMeshes].InstanceData.Transform = transform;
    meshDrawData[numMeshes].InstanceData.MaterialID = s_Data.NumMaterials;

    numMeshes++;
    ASSERT(numMeshes <= MAX_MESHES, "Exceeded the maximum amount of meshes");

    s_Data.NumMaterials++;
    ASSERT(s_Data.NumMaterials <= MAX_MATERIALS, "Exceeded the maximum amount of materials");
}

void Renderer::Submit(const DirectionalLightData& dirLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap)
{
    s_Data.DirectionalLightDrawData[s_Data.SceneData.NumDirLights] = dirLightData;
    s_Data.LightCameras[s_Data.SceneData.NumDirLights] = lightCamera;
    s_Data.LightShadowMaps[s_Data.SceneData.NumDirLights] = shadowMap;

    s_Data.SceneData.NumDirLights++;
    ASSERT(s_Data.SceneData.NumDirLights <= MAX_DIR_LIGHTS, "Exceeded the maximum amount of directional lights");
}

void Renderer::Submit(const PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, const std::shared_ptr<Texture>& shadowMap)
{
    std::size_t pointLightBaseIndex = MAX_DIR_LIGHTS;

    s_Data.PointLightDrawData[s_Data.SceneData.NumPointLights] = pointLightData;
    for (uint32_t i = 0; i < 6; ++i)
        s_Data.LightCameras[pointLightBaseIndex + (s_Data.SceneData.NumPointLights * 6) + i] = lightCameras[i];
    s_Data.LightShadowMaps[pointLightBaseIndex + s_Data.SceneData.NumPointLights] = shadowMap;

    s_Data.SceneData.NumPointLights++;
    ASSERT(s_Data.SceneData.NumPointLights <= MAX_POINT_LIGHTS, "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const SpotLightData& spotLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap)
{
    std::size_t spotLightBaseIndex = MAX_DIR_LIGHTS + MAX_POINT_LIGHTS;

    s_Data.SpotLightDrawData[s_Data.SceneData.NumSpotLights] = spotLightData;
    s_Data.LightCameras[MAX_DIR_LIGHTS + (MAX_POINT_LIGHTS * 6) + s_Data.SceneData.NumSpotLights] = lightCamera;
    s_Data.LightShadowMaps[spotLightBaseIndex + s_Data.SceneData.NumSpotLights] = shadowMap;

    s_Data.SceneData.NumSpotLights++;
    ASSERT(s_Data.SceneData.NumSpotLights <= MAX_SPOT_LIGHTS, "Exceeded the maximum amount of spot lights");
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (s_Data.RenderSettings.RenderResolution.x != width || s_Data.RenderSettings.RenderResolution.y != height)
    {
        s_Data.RenderSettings.RenderResolution.x = std::max(1u, width);
        s_Data.RenderSettings.RenderResolution.y = std::max(1u, height);

        RenderBackend::Resize(width, height);

        for (auto& frameBuffer : s_Data.FrameBuffers)
        {
            if (frameBuffer)
            {
                frameBuffer->Resize(s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
            }
        }

        s_Data.SDRRenderTarget->Resize(width, height);
    }
}

void Renderer::ToggleVSync()
{
    s_Data.RenderSettings.VSync = !s_Data.RenderSettings.VSync;
    RenderBackend::SetVSync(s_Data.RenderSettings.VSync);
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
    return *s_Data.SDRRenderTarget;
}

// Temp
Texture& Renderer::GetFinalDepthOutput()
{
    return s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS]->GetDepthAttachment();
}

void Renderer::MakeRenderPasses()
{
    {
        // Shadow mapping render pass
        RasterPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ShadowMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ShadowMapping_PS.hlsl";
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.ShadowMapResolution.x, s_Data.RenderSettings.ShadowMapResolution.y);
        desc.DepthBias = -50;
        desc.SlopeScaledDepthBias = -5.0f;
        desc.DepthBiasClamp = 1.0f;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstants(16, 0); // Light VP

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING] = std::make_unique<RasterPass>("Shadow mapping", desc);
    }

    {
        // Depth pre-pass
        RasterPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DepthPrepass_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DepthPrepass_PS.hlsl";
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstantBufferView(0); // Scene data

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::DEPTH_PREPASS] = std::make_unique<RasterPass>("Depth pre-pass", desc);
    }

    {
        // Lighting render pass
        RasterPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Lighting_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Lighting_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
            TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            TextureDimension::TEXTURE_DIMENSION_2D, 0, 0);
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        desc.RootParameters.resize(4);
        desc.RootParameters[0].InitAsConstantBufferView(0); // Scene data
        desc.RootParameters[1].InitAsConstantBufferView(1); // Material constant buffer
        desc.RootParameters[2].InitAsConstantBufferView(2); // Light constant buffer
        desc.RootParameters[3].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Bindless SRV table

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MATERIAL_ID", 0, DXGI_FORMAT_R32_UINT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::LIGHTING] = std::make_unique<RasterPass>("Lighting", desc);
    }

    {
        // Tonemapping render pass
        ComputePassDesc desc;
        desc.ComputeShaderPath = "Resources/Shaders/Tonemapping_CS.hlsl";
        desc.NumThreadsX = 64;
        desc.NumThreadsY = 64;
        desc.NumThreadsZ = 1;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4096, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

        desc.RootParameters.resize(3);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // Tonemapping settings
        desc.RootParameters[1].InitAsDescriptorTable(1, &ranges[0]); // Bindless SRV table
        desc.RootParameters[2].InitAsDescriptorTable(1, &ranges[1]); // Bindless UAV table

        s_Data.ComputePasses[ComputePassType::TONE_MAPPING] = std::make_unique<ComputePass>("Tonemapping", desc);
    }
}

void Renderer::MakeBuffers()
{
    for (uint32_t i = 0; i < TransparencyMode::NUM_ALPHA_MODES; ++i)
    {
        s_Data.MeshInstanceBuffer[i] = std::make_unique<Buffer>("Mesh instance buffer", BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD,
            MAX_MESHES * RenderBackend::GetSwapChain().GetBackBufferCount(), sizeof(MeshInstanceData)));
    }

    s_Data.MaterialConstantBuffer = std::make_unique<Buffer>("Material constant buffer",
        BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, MAX_MATERIALS, sizeof(MaterialData)));

    std::size_t totalLightBufferByteSize = MAX_DIR_LIGHTS * sizeof(DirectionalLightData) + MAX_SPOT_LIGHTS * sizeof(SpotLightData) + MAX_POINT_LIGHTS * sizeof(PointLightData);
    s_Data.LightConstantBuffer = std::make_unique<Buffer>("Light constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, totalLightBufferByteSize));

    s_Data.TonemapConstantBuffer = std::make_unique<Buffer>("Tonemap constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(TonemapSettings)));
    s_Data.SceneDataConstantBuffer = std::make_unique<Buffer>("Scene data constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
}

void Renderer::MakeFrameBuffers()
{
    FrameBufferDesc shadowMappingDesc = {};
    shadowMappingDesc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.ShadowMapResolution.x, s_Data.RenderSettings.ShadowMapResolution.y, glm::vec2(0.0f, 0.0f));
    s_Data.FrameBuffers[RenderPassType::SHADOW_MAPPING] = std::make_shared<FrameBuffer>("Shadow mapping frame buffer", shadowMappingDesc);

    FrameBufferDesc depthPrepassDesc = {};
    depthPrepassDesc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y, glm::vec2(1.0f, 0.0f));
    s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS] = std::make_shared<FrameBuffer>("Depth pre-pass frame buffer", depthPrepassDesc);

    FrameBufferDesc hdrDesc = {};
    hdrDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
        TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    s_Data.FrameBuffers[RenderPassType::LIGHTING] = std::make_shared<FrameBuffer>("HDR frame buffer", hdrDesc);

    s_Data.SDRRenderTarget = std::make_unique<Texture>("SDR render target", TextureDesc(TextureUsage::TEXTURE_USAGE_WRITE, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
        TextureDimension::TEXTURE_DIMENSION_2D, s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y));
}

void Renderer::PrepareInstanceBuffer()
{
    uint32_t currentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();

    for (uint32_t i = 0; i < TransparencyMode::NUM_ALPHA_MODES; ++i)
    {
        std::size_t currentByteOffset = static_cast<std::size_t>(currentBackBufferIndex * MAX_MESHES) * sizeof(MeshInstanceData);
        Buffer& instanceBuffer = *s_Data.MeshInstanceBuffer[i];

        for (auto& [primitive, instanceData] : s_Data.MeshDrawData[i])
        {
            std::size_t numBytes = sizeof(MeshInstanceData);
            instanceBuffer.SetBufferDataAtOffset(&instanceData, numBytes, currentByteOffset);
            currentByteOffset += numBytes;
        }
    }
}

void Renderer::PrepareLightBuffers()
{
    // Set constant buffer data for directional lights/pointlights/spotlights
    std::size_t dirLightsByteStart = 0;
    std::size_t spotLightsByteStart = MAX_DIR_LIGHTS * sizeof(DirectionalLightData);
    std::size_t pointLightsByteStart = spotLightsByteStart + MAX_SPOT_LIGHTS * sizeof(SpotLightData);

    if (s_Data.SceneData.NumDirLights > 0)
        s_Data.LightConstantBuffer->SetBufferDataAtOffset(&s_Data.DirectionalLightDrawData[0], s_Data.SceneData.NumDirLights * sizeof(DirectionalLightData), dirLightsByteStart);

    if (s_Data.SceneData.NumSpotLights > 0)
        s_Data.LightConstantBuffer->SetBufferDataAtOffset(&s_Data.SpotLightDrawData[0], s_Data.SceneData.NumSpotLights * sizeof(SpotLightData), spotLightsByteStart);

    if (s_Data.SceneData.NumPointLights > 0)
        s_Data.LightConstantBuffer->SetBufferDataAtOffset(&s_Data.PointLightDrawData[0], s_Data.SceneData.NumPointLights * sizeof(PointLightData), pointLightsByteStart);
}

void Renderer::RenderShadowMap(CommandList& commandList, const Camera& lightCamera, const Texture& shadowMap, uint32_t descriptorOffset)
{
    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(shadowMap.GetTextureDesc().Width),
        static_cast<float>(shadowMap.GetTextureDesc().Height), 0.0f, 1.0f);
    CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

    commandList.SetViewports(1, &viewport);
    commandList.SetScissorRects(1, &scissorRect);

    D3D12_CPU_DESCRIPTOR_HANDLE dsv = shadowMap.GetDescriptor(DescriptorType::DSV, descriptorOffset);
    commandList.SetRenderTargets(0, nullptr, &dsv);

    // Set root constant (light VP)
    const glm::mat4& lightViewProjection = lightCamera.GetViewProjection();
    commandList.SetRootConstants(0, 16, &lightViewProjection[0][0], 0);

    RenderGeometry(commandList, lightCamera, TransparencyMode::OPAQUE);
    RenderGeometry(commandList, lightCamera, TransparencyMode::TRANSPARENT);
}

void Renderer::RenderGeometry(CommandList& commandList, const Camera& camera, uint32_t alphaMode)
{
    // Set instance buffer
    commandList.SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer[alphaMode]);
    uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * MAX_MESHES;

    std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

    for (uint32_t j = 0; j < s_Data.NumMeshes[alphaMode]; ++j)
    {
        auto& primitive = s_Data.MeshDrawData[alphaMode][j].Primitive;
        auto& instanceData = s_Data.MeshDrawData[alphaMode][j].InstanceData;

        // Cull mesh from light camera frustum
        if (camera.IsFrustumCullingEnabled())
        {
            if (!camera.GetViewFrustum().IsBoxInViewFrustum(primitive.BB.Min, primitive.BB.Max))
            {
                startInstance++;
                continue;
            }
        }

        auto& vb = primitive.VertexBuffer;
        auto& ib = primitive.IndexBuffer;

        if (vb != currentVertexBuffer)
        {
            commandList.SetVertexBuffers(0, 1, *vb);
            currentVertexBuffer = vb;
        }
        if (ib != currentIndexBuffer)
        {
            commandList.SetIndexBuffer(*ib);
            currentIndexBuffer = ib;
        }

        commandList.DrawIndexed(static_cast<uint32_t>(primitive.NumIndices), 1,
            static_cast<uint32_t>(primitive.IndicesStart), static_cast<int32_t>(primitive.VerticesStart), startInstance);

        startInstance++;
        s_Data.RenderStatistics.DrawCallCount++;
    }
}
