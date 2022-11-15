#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Mesh.h"
#include "Graphics/Shader.h"
#include "Resource/Model.h"
#include "Graphics/RenderPass.h"
#include "Graphics/FrameBuffer.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/Device.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/RenderBackend.h"

#include <imgui/imgui.h>

struct SceneData
{
    void Reset()
    {
        NumDirLights = 0;
        NumPointLights = 0;
        NumSpotLights = 0;
    }

    glm::mat4 SceneCameraViewProjection = glm::identity<glm::mat4>();
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
    TONE_MAPPING,
    NUM_RENDER_PASSES = (TONE_MAPPING + 1)
};

struct MeshInstanceData
{
    MeshInstanceData() = default;
    MeshInstanceData(const glm::mat4& transform, const glm::vec4& color, uint32_t baseColorTexIndex, uint32_t normalTexIndex)
        : Transform(transform), Color(color), BaseColorTexIndex(baseColorTexIndex), NormalTexIndex(normalTexIndex) {}

    glm::mat4 Transform = glm::identity<glm::mat4>();
    glm::vec4 Color = glm::vec4(1.0f);
    uint32_t BaseColorTexIndex = 0;
    uint32_t NormalTexIndex = 0;
};

struct MeshDrawData
{
    std::shared_ptr<Mesh> Mesh;
    MeshInstanceData InstanceData;
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
    std::unique_ptr<RenderPass> RenderPasses[RenderPassType::NUM_RENDER_PASSES];
    std::array<std::shared_ptr<FrameBuffer>, RenderPassType::NUM_RENDER_PASSES> FrameBuffers;

    // Renderer settings and statistics
    Renderer::RenderSettings RenderSettings;
    RendererStatistics RenderStatistics;

    // Tone mapping settings and buffers
    TonemapSettings TonemapSettings;
    std::unique_ptr<Buffer> TonemapConstantBuffer;
    std::unique_ptr<Buffer> TonemapVertexBuffer;
    std::unique_ptr<Buffer> TonemapIndexBuffer;

    // Scene data and buffer
    Camera SceneCamera;
    SceneData SceneData;
    std::unique_ptr<Buffer> SceneDataConstantBuffer;

    // Mesh draw data and buffer
    std::unique_ptr<Buffer> MeshInstanceBuffer;
    std::array<MeshDrawData, 1000> MeshDrawData;
    std::size_t NumMeshes = 0;

    // Directional/point/spotlight constant buffers
    std::unique_ptr<Buffer> DirectionalLightConstantBuffer;
    std::unique_ptr<Buffer> PointLightConstantBuffer;
    std::unique_ptr<Buffer> SpotLightConstantBuffer;

    // Directional/point/spotlight draw data
    std::array<DirectionalLightData, 1> DirectionalLightDrawData;
    std::array<PointLightData, 50> PointLightDrawData;
    std::array<SpotLightData, 50> SpotLightDrawData;

    // Light cameras and light shadow maps
    std::array<Camera, 1 + 6 * 50 + 50> LightCameras;
    std::array<std::shared_ptr<Texture>, 101> LightShadowMaps;
    
    uint32_t CurrentBackBufferIndex = 0;
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
}

void Renderer::Finalize()
{
    RenderBackend::Finalize();
}

void Renderer::BeginScene(const Camera& sceneCamera, const glm::vec3& ambient)
{
    SCOPED_TIMER("Renderer::BeginScene");

    s_Data.CurrentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();
    
    s_Data.SceneCamera = sceneCamera;
    s_Data.SceneData.SceneCameraViewProjection = sceneCamera.GetViewProjection();
    s_Data.SceneData.Ambient = ambient;

    s_Data.TonemapSettings.Exposure = sceneCamera.GetExposure();
    s_Data.TonemapSettings.Gamma = sceneCamera.GetGamma();

    for (auto& frameBuffer : s_Data.FrameBuffers)
    {
        if (frameBuffer)
            frameBuffer->ClearAttachments();
    }
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    s_Data.RenderStatistics.MeshCount = s_Data.NumMeshes;
    for (uint32_t i = 0; i < s_Data.NumMeshes; ++i)
        s_Data.RenderStatistics.TriangleCount += s_Data.MeshDrawData[i].Mesh->GetNumIndices() / 3;

    PrepareInstanceBuffer();
    PrepareLightBuffers();
    PrepareShadowMaps();

    auto& descriptorHeap = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
    s_Data.SceneDataConstantBuffer->SetBufferData(&s_Data.SceneData);

    CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.RenderResolution.x), static_cast<float>(s_Data.RenderSettings.RenderResolution.y), 0.0f, 1.0f);
    CD3DX12_RECT scissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    {
        /* Shadow mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Shadow mapping");

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        CD3DX12_VIEWPORT shadowmapViewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Data.RenderSettings.ShadowMapResolution.x),
            static_cast<float>(s_Data.RenderSettings.ShadowMapResolution.y), 0.0f, 1.0f);
        commandList->SetViewports(1, &shadowmapViewport);
        commandList->SetScissorRects(1, &scissorRect);

        std::size_t cameraIndex = 0, shadowMapIndex = 0;

        for (uint32_t i = 0; i < s_Data.SceneData.NumDirLights; ++i)
        {
            RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], s_Data.LightShadowMaps[shadowMapIndex]->GetDescriptor(DescriptorType::DSV));

            cameraIndex++;
            shadowMapIndex++;
        }

        cameraIndex = s_Data.RenderSettings.MaxDirectionalLights;
        shadowMapIndex = s_Data.RenderSettings.MaxDirectionalLights;

        for (uint32_t i = 0; i < s_Data.SceneData.NumPointLights; ++i)
        {
            for (uint32_t face = 0; face < 6; ++face)
            {
                RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], s_Data.LightShadowMaps[shadowMapIndex]->GetDescriptor(DescriptorType::DSV, 1 + face));
                cameraIndex++;
            }

            shadowMapIndex++;
        }

        cameraIndex = static_cast<std::size_t>(s_Data.RenderSettings.MaxDirectionalLights + (s_Data.RenderSettings.MaxPointLights * 6));
        shadowMapIndex = static_cast<std::size_t>(s_Data.RenderSettings.MaxDirectionalLights + s_Data.RenderSettings.MaxPointLights);

        for (uint32_t i = 0; i < s_Data.SceneData.NumSpotLights; ++i)
        {
            RenderShadowMap(*commandList, s_Data.LightCameras[cameraIndex], s_Data.LightShadowMaps[shadowMapIndex]->GetDescriptor(DescriptorType::DSV));

            cameraIndex++;
            shadowMapIndex++;
        }

        commandList->EndTimestampQuery("Shadow mapping");
        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Depth pre-pass render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Depth pre-pass");

        auto& depthPrepassDepthTarget = s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS]->GetDepthAttachment();
        auto dsv = depthPrepassDepthTarget.GetDescriptor(DescriptorType::DSV);

        commandList->SetViewports(1, &viewport);
        commandList->SetScissorRects(1, &scissorRect);
        commandList->SetRenderTargets(0, nullptr, &dsv);

        commandList->SetPipelineState(s_Data.RenderPasses[RenderPassType::DEPTH_PREPASS]->GetPipelineState());

        commandList->SetRootConstantBufferView(0, *s_Data.SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set instance buffer
        commandList->SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer);
        uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * s_Data.RenderSettings.MaxMeshes;

        std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

        for (uint32_t i = 0; i < s_Data.NumMeshes; ++i)
        {
            auto& mesh = s_Data.MeshDrawData[i].Mesh;
            auto& instanceData = s_Data.MeshDrawData[i].InstanceData;

            // Cull mesh from light camera frustum
            if (s_Data.SceneCamera.IsFrustumCullingEnabled())
            {
                BoundingBox boundingBox = mesh->GetBoundingBox();

                boundingBox.Min = glm::vec4(boundingBox.Min, 1.0f) * instanceData.Transform;
                boundingBox.Max = glm::vec4(boundingBox.Max, 1.0f) * instanceData.Transform;

                if (!s_Data.SceneCamera.GetViewFrustum().IsBoxInViewFrustum(boundingBox.Min, boundingBox.Max))
                {
                    startInstance++;
                    continue;
                }
            }

            auto vb = mesh->GetVertexBuffer();
            auto ib = mesh->GetIndexBuffer();

            if (vb != currentVertexBuffer)
            {
                commandList->SetVertexBuffers(0, 1, *vb);
                currentVertexBuffer = vb;
            }
            if (ib != currentIndexBuffer)
            {
                commandList->SetIndexBuffer(*ib);
                currentIndexBuffer = ib;
            }

            commandList->DrawIndexed(static_cast<uint32_t>(mesh->GetNumIndices()), 1,
                static_cast<uint32_t>(mesh->GetStartIndex()), static_cast<int32_t>(mesh->GetStartVertex()), startInstance);
        }

        commandList->EndTimestampQuery("Depth pre-pass");
        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Lighting render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Lighting");

        auto& hdrColorTarget = s_Data.FrameBuffers[RenderPassType::LIGHTING]->GetColorAttachment();
        auto& depthPrepassDepthTarget = s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS]->GetDepthAttachment();

        auto rtv = hdrColorTarget.GetDescriptor(DescriptorType::RTV);
        auto dsv = depthPrepassDepthTarget.GetDescriptor(DescriptorType::DSV);

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &viewport);
        commandList->SetScissorRects(1, &scissorRect);
        commandList->SetRenderTargets(1, &rtv, &dsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set the pipeline state along with the root signature
        commandList->SetPipelineState(s_Data.RenderPasses[RenderPassType::LIGHTING]->GetPipelineState());

        // Set root CBVs for scene data
        commandList->SetRootConstantBufferView(0, *s_Data.SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set root CBVs for lights
        commandList->SetRootConstantBufferView(1, *s_Data.DirectionalLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        commandList->SetRootConstantBufferView(2, *s_Data.PointLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        commandList->SetRootConstantBufferView(3, *s_Data.SpotLightConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(4, descriptorHeap.GetGPUBaseDescriptor());

        // Set instance buffer
        commandList->SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer);
        uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * s_Data.RenderSettings.MaxMeshes;

        std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

        for (uint32_t i = 0; i < s_Data.NumMeshes; ++i)
        {
            auto& mesh = s_Data.MeshDrawData[i].Mesh;
            auto& instanceData = s_Data.MeshDrawData[i].InstanceData;

            // Cull mesh from light camera frustum
            if (s_Data.SceneCamera.IsFrustumCullingEnabled())
            {
                BoundingBox boundingBox = mesh->GetBoundingBox();

                boundingBox.Min = glm::vec4(boundingBox.Min, 1.0f) * instanceData.Transform;
                boundingBox.Max = glm::vec4(boundingBox.Max, 1.0f) * instanceData.Transform;

                if (!s_Data.SceneCamera.GetViewFrustum().IsBoxInViewFrustum(boundingBox.Min, boundingBox.Max))
                {
                    startInstance++;
                    continue;
                }
            }

            auto vb = mesh->GetVertexBuffer();
            auto ib = mesh->GetIndexBuffer();

            if (vb != currentVertexBuffer)
            {
                commandList->SetVertexBuffers(0, 1, *vb);
                currentVertexBuffer = vb;
            }
            if (ib != currentIndexBuffer)
            {
                commandList->SetIndexBuffer(*ib);
                currentIndexBuffer = ib;
            }

            /*commandList->DrawIndexed(static_cast<uint32_t>(mesh->GetNumIndices()), static_cast<uint32_t>(instanceData.size()),
                static_cast<uint32_t>(mesh->GetStartIndex()), static_cast<int32_t>(mesh->GetStartVertex()), startInstance);
            startInstance += static_cast<uint32_t>(meshInstanceData.size());*/
            commandList->DrawIndexed(static_cast<uint32_t>(mesh->GetNumIndices()), 1,
                static_cast<uint32_t>(mesh->GetStartIndex()), static_cast<int32_t>(mesh->GetStartVertex()), startInstance);

            startInstance++;
            s_Data.RenderStatistics.DrawCallCount++;
        }

        commandList->EndTimestampQuery("Lighting");
        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Tone mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Tonemapping");

        auto& tmColorTarget = s_Data.FrameBuffers[RenderPassType::TONE_MAPPING]->GetColorAttachment();
        auto& tmDepthBuffer = s_Data.FrameBuffers[RenderPassType::TONE_MAPPING]->GetDepthAttachment();

        auto tmRtv = tmColorTarget.GetDescriptor(DescriptorType::RTV);
        auto tmDsv = tmDepthBuffer.GetDescriptor(DescriptorType::DSV);

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &viewport);
        commandList->SetScissorRects(1, &scissorRect);
        commandList->SetRenderTargets(1, &tmRtv, &tmDsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set pipeline state and root signature
        commandList->SetPipelineState(s_Data.RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

        uint32_t pbrColorTargetIndex = s_Data.FrameBuffers[RenderPassType::LIGHTING]->GetColorAttachment().GetDescriptorHeapIndex(DescriptorType::SRV);
        s_Data.TonemapSettings.HDRTargetIndex = pbrColorTargetIndex;
        s_Data.TonemapConstantBuffer->SetBufferData(&s_Data.TonemapSettings);
        commandList->SetRootConstantBufferView(0, *s_Data.TonemapConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(1, descriptorHeap.GetGPUBaseDescriptor());

        // Set tone mapping vertex buffer
        commandList->SetVertexBuffers(0, 1, *s_Data.TonemapVertexBuffer.get());
        commandList->SetIndexBuffer(*s_Data.TonemapIndexBuffer.get());

        commandList->DrawIndexed(static_cast<uint32_t>(s_Data.TonemapIndexBuffer->GetBufferDesc().NumElements), 1);

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

    if (ImGui::CollapsingHeader("Max settings"))
    {
        ImGui::Text("Max model instances: %u", s_Data.RenderSettings.MaxInstances);
        ImGui::Text("Max instances per draw: %u", s_Data.RenderSettings.MaxMeshes);
        ImGui::Text("Max directional lights: %u", s_Data.RenderSettings.MaxDirectionalLights);
        ImGui::Text("Max point lights: %u", s_Data.RenderSettings.MaxPointLights);
        ImGui::Text("Max spot lights: %u", s_Data.RenderSettings.MaxSpotLights);
    }

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

    RenderBackend::GetSwapChain().ResolveToBackBuffer(s_Data.FrameBuffers[RenderPassType::TONE_MAPPING]->GetColorAttachment());

    s_Data.NumMeshes = 0;
    s_Data.SceneData.Reset();
    s_Data.RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    uint32_t baseColorTexIndex = mesh->GetTexture(MeshTextureType::TEX_BASE_COLOR)->GetDescriptorHeapIndex(DescriptorType::SRV);
    uint32_t normalTexIndex = mesh->GetTexture(MeshTextureType::TEX_NORMAL)->GetDescriptorHeapIndex(DescriptorType::SRV);

    s_Data.MeshDrawData[s_Data.NumMeshes].Mesh = mesh;
    s_Data.MeshDrawData[s_Data.NumMeshes].InstanceData.Transform = transform;
    s_Data.MeshDrawData[s_Data.NumMeshes].InstanceData.BaseColorTexIndex = baseColorTexIndex;
    s_Data.MeshDrawData[s_Data.NumMeshes].InstanceData.NormalTexIndex = normalTexIndex;

    s_Data.NumMeshes++;
    ASSERT(s_Data.NumMeshes <= s_Data.RenderSettings.MaxMeshes, "Exceeded the maximum amount of meshes");
}

void Renderer::Submit(const DirectionalLightData& dirLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap)
{
    s_Data.DirectionalLightDrawData[s_Data.SceneData.NumDirLights] = dirLightData;
    s_Data.LightCameras[s_Data.SceneData.NumDirLights] = lightCamera;
    s_Data.LightShadowMaps[s_Data.SceneData.NumDirLights] = shadowMap;

    s_Data.SceneData.NumDirLights++;
    ASSERT(s_Data.SceneData.NumDirLights <= s_Data.RenderSettings.MaxDirectionalLights, "Exceeded the maximum amount of directional lights");
}

void Renderer::Submit(const PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, const std::shared_ptr<Texture>& shadowMap)
{
    std::size_t pointLightBaseIndex = s_Data.RenderSettings.MaxDirectionalLights;

    s_Data.PointLightDrawData[s_Data.SceneData.NumPointLights] = pointLightData;
    for (uint32_t i = 0; i < 6; ++i)
        s_Data.LightCameras[pointLightBaseIndex + (s_Data.SceneData.NumPointLights * 6) + i] = lightCameras[i];
    s_Data.LightShadowMaps[pointLightBaseIndex + s_Data.SceneData.NumPointLights] = shadowMap;

    s_Data.SceneData.NumPointLights++;
    ASSERT(s_Data.SceneData.NumPointLights <= s_Data.RenderSettings.MaxPointLights, "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const SpotLightData& spotLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap)
{
    std::size_t spotLightBaseIndex = s_Data.RenderSettings.MaxDirectionalLights + s_Data.RenderSettings.MaxPointLights;

    s_Data.SpotLightDrawData[s_Data.SceneData.NumSpotLights] = spotLightData;
    s_Data.LightCameras[s_Data.RenderSettings.MaxDirectionalLights + (s_Data.RenderSettings.MaxPointLights * 6) + s_Data.SceneData.NumSpotLights] = lightCamera;
    s_Data.LightShadowMaps[spotLightBaseIndex + s_Data.SceneData.NumSpotLights] = shadowMap;

    s_Data.SceneData.NumSpotLights++;
    ASSERT(s_Data.SceneData.NumSpotLights <= s_Data.RenderSettings.MaxSpotLights, "Exceeded the maximum amount of spot lights");
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
    return s_Data.FrameBuffers[RenderPassType::TONE_MAPPING]->GetColorAttachment();
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
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ShadowMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ShadowMapping_PS.hlsl";
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            0, 0);
        desc.DepthBias = -50;
        desc.SlopeScaledDepthBias = -5.0f;
        desc.DepthBiasClamp = 1.0f;

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
        // Depth pre-pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DepthPrepass_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DepthPrepass_PS.hlsl";
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstantBufferView(0); // Scene data

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 1, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "TEX_INDICES", 0, DXGI_FORMAT_R32G32_UINT, 1, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Data.RenderPasses[RenderPassType::DEPTH_PREPASS] = std::make_unique<RenderPass>("Depth pre-pass", desc);
    }

    {
        // Lighting render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Lighting_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Lighting_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
            s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_NONE, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            0, 0);
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 512, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
        ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 512, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

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
            s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
        desc.DepthEnabled = false;
        desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS;

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
        s_Data.RenderSettings.MaxMeshes * RenderBackend::GetSwapChain().GetBackBufferCount(), sizeof(MeshInstanceData)));

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

void Renderer::MakeFrameBuffers()
{
    FrameBufferDesc depthPrepassDesc = {};
    depthPrepassDesc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    s_Data.FrameBuffers[RenderPassType::DEPTH_PREPASS] = std::make_shared<FrameBuffer>("Depth pre-pass frame buffer", depthPrepassDesc);

    FrameBufferDesc hdrDesc = {};
    hdrDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
        s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    s_Data.FrameBuffers[RenderPassType::LIGHTING] = std::make_shared<FrameBuffer>("HDR frame buffer", hdrDesc);

    FrameBufferDesc tonemapDesc = {};
    tonemapDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
        s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    tonemapDesc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        s_Data.RenderSettings.RenderResolution.x, s_Data.RenderSettings.RenderResolution.y);
    s_Data.FrameBuffers[RenderPassType::TONE_MAPPING] = std::make_shared<FrameBuffer>("Tonemap color target", tonemapDesc);
}

void Renderer::PrepareInstanceBuffer()
{
    uint32_t currentBackBufferIndex = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex();
    std::size_t currentByteOffset = static_cast<std::size_t>(currentBackBufferIndex * s_Data.RenderSettings.MaxMeshes) * sizeof(MeshInstanceData);

    for (auto& [mesh, instanceData] : s_Data.MeshDrawData)
    {
        std::size_t numBytes = sizeof(MeshInstanceData);
        s_Data.MeshInstanceBuffer->SetBufferDataAtOffset(&instanceData, numBytes, currentByteOffset);
        currentByteOffset += numBytes;
    }
}

void Renderer::PrepareLightBuffers()
{
    // Set constant buffer data for directional lights/pointlights/spotlights
    if (s_Data.SceneData.NumDirLights > 0)
        s_Data.DirectionalLightConstantBuffer->SetBufferData(&s_Data.DirectionalLightDrawData[0], s_Data.DirectionalLightDrawData.size() * sizeof(DirectionalLightData));

    if (s_Data.SceneData.NumPointLights > 0)
        s_Data.PointLightConstantBuffer->SetBufferData(&s_Data.PointLightDrawData[0], s_Data.PointLightDrawData.size() * sizeof(PointLightData));

    if (s_Data.SceneData.NumSpotLights > 0)
        s_Data.SpotLightConstantBuffer->SetBufferData(&s_Data.SpotLightDrawData[0], s_Data.SpotLightDrawData.size() * sizeof(SpotLightData));
}

void Renderer::PrepareShadowMaps()
{
    auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (auto& shadowMap : s_Data.LightShadowMaps)
    {
        if (shadowMap)
            commandList->ClearDepthStencilView(shadowMap->GetDescriptor(DescriptorType::DSV), 0.0f);
    }

    RenderBackend::ExecuteCommandListAndWait(commandList);
}

void Renderer::RenderShadowMap(CommandList& commandList, const Camera& lightCamera, D3D12_CPU_DESCRIPTOR_HANDLE shadowMapDepthView)
{
    commandList.SetRenderTargets(0, nullptr, &shadowMapDepthView);

    // Set the pipeline state along with the root signature
    commandList.SetPipelineState(s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING]->GetPipelineState());

    // Set root constant (light VP)
    const glm::mat4& lightViewProjection = lightCamera.GetViewProjection();
    commandList.SetRootConstants(0, 16, &lightViewProjection[0][0], 0);

    // Set instance buffer
    commandList.SetVertexBuffers(1, 1, *s_Data.MeshInstanceBuffer);
    uint32_t startInstance = RenderBackend::GetSwapChain().GetCurrentBackBufferIndex() * s_Data.RenderSettings.MaxMeshes;

    std::shared_ptr<Buffer> currentVertexBuffer = nullptr, currentIndexBuffer = nullptr;

    for (uint32_t i = 0; i < s_Data.NumMeshes; ++i)
    {
        auto& mesh = s_Data.MeshDrawData[i].Mesh;
        auto& instanceData = s_Data.MeshDrawData[i].InstanceData;

        // Cull mesh from light camera frustum
        /*if (lightCamera.IsFrustumCullingEnabled())
        {
            Mesh::BoundingBox boundingBox = mesh->GetBoundingBox();
            
            boundingBox.Min = instanceData.Transform * glm::vec4(boundingBox.Min, 1.0f);
            boundingBox.Max = instanceData.Transform * glm::vec4(boundingBox.Max, 1.0f);

            if (!lightCamera.GetViewFrustum().IsBoxInViewFrustum(boundingBox.Min, boundingBox.Max))
            {
                startInstance++;
                continue;
            }
        }*/

        auto vb = mesh->GetVertexBuffer();
        auto ib = mesh->GetIndexBuffer();

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

        /*commandList.DrawIndexed(static_cast<uint32_t>(mesh->GetNumIndices()), static_cast<uint32_t>(meshInstanceData.size()),
            static_cast<uint32_t>(mesh->GetStartIndex()), static_cast<int32_t>(mesh->GetStartVertex()), startInstance);
        startInstance += static_cast<uint32_t>(meshInstanceData.size());*/
        commandList.DrawIndexed(static_cast<uint32_t>(mesh->GetNumIndices()), 1,
            static_cast<uint32_t>(mesh->GetStartIndex()), static_cast<int32_t>(mesh->GetStartVertex()), startInstance);
        startInstance += 1;

        s_Data.RenderStatistics.DrawCallCount++;
    }
}
