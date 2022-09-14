#include "Pch.h"
#include "Graphics/Renderer.h"
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

static Renderer* s_Instance = nullptr;

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    if (!s_Instance)
        s_Instance = new Renderer();

    RenderBackend::Get().Initialize(hWnd, width, height);

    s_Instance->m_RenderSettings.Resolution.x = width;
    s_Instance->m_RenderSettings.Resolution.y = height;

    s_Instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Instance->m_RenderSettings.Resolution.x), static_cast<float>(s_Instance->m_RenderSettings.Resolution.y), 0.0f, 1.0f);
    s_Instance->m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    MakeRenderPasses();
    MakeBuffers();
}

void Renderer::Finalize()
{
    RenderBackend::Get().Finalize();
    delete s_Instance;
}

void Renderer::BeginScene(const Camera& sceneCamera, const glm::vec3& ambient)
{
    SCOPED_TIMER("Renderer::BeginScene");
    
    s_Instance->m_SceneData.ViewProjection = sceneCamera.GetViewProjection();
    s_Instance->m_SceneData.Ambient = ambient;

    s_Instance->m_TonemapSettings.Exposure = sceneCamera.GetExposure();
    s_Instance->m_TonemapSettings.Gamma = sceneCamera.GetGamma();

    auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

    for (auto& renderPass : s_Instance->m_RenderPasses)
        renderPass->ClearViews(commandList);

    RenderBackend::Get().ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    auto& descriptorHeap = *RenderBackend::Get().GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).get();

    {
        /* Default render pass (lighting) */
        auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        auto& defaultColorTarget = s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
        auto& defaultDepthBuffer = s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetDepthAttachment();

        auto rtv = defaultColorTarget.GetRenderTargetDepthStencilView();
        auto dsv = defaultDepthBuffer.GetRenderTargetDepthStencilView();

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &s_Instance->m_Viewport);
        commandList->SetScissorRects(1, &s_Instance->m_ScissorRect);
        commandList->SetRenderTargets(1, &rtv, &dsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set pipeline state, root signature, and scene data constant buffer
        commandList->SetPipelineState(s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetPipelineState());

        s_Instance->m_SceneData.NumDirLights = s_Instance->m_DirectionalLightDrawData.size();
        s_Instance->m_SceneData.NumPointLights = s_Instance->m_PointLightDrawData.size();
        s_Instance->m_SceneData.NumSpotLights = s_Instance->m_SpotLightDrawData.size();

        s_Instance->m_SceneDataConstantBuffer->SetBufferData(&s_Instance->m_SceneData);
        commandList->SetRootConstantBufferView(0, *s_Instance->m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set constant buffer data for lights
        if (s_Instance->m_SceneData.NumDirLights > 0)
            s_Instance->m_DirectionalLightBuffer->SetBufferData(&s_Instance->m_DirectionalLightDrawData[0], s_Instance->m_DirectionalLightDrawData.size() * sizeof(DirectionalLightData));

        commandList->SetRootConstantBufferView(1, *s_Instance->m_DirectionalLightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        s_Instance->m_RenderStatistics.DirectionalLightCount += s_Instance->m_DirectionalLightDrawData.size();

        if (s_Instance->m_SceneData.NumPointLights > 0)
            s_Instance->m_PointLightBuffer->SetBufferData(&s_Instance->m_PointLightDrawData[0], s_Instance->m_PointLightDrawData.size() * sizeof(PointLightData));

        commandList->SetRootConstantBufferView(2, *s_Instance->m_PointLightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        s_Instance->m_RenderStatistics.PointLightCount += s_Instance->m_PointLightDrawData.size();

        if (s_Instance->m_SceneData.NumSpotLights > 0)
            s_Instance->m_SpotLightBuffer->SetBufferData(&s_Instance->m_SpotLightDrawData[0], s_Instance->m_SpotLightDrawData.size() * sizeof(SpotLightData));

        commandList->SetRootConstantBufferView(3,*s_Instance->m_SpotLightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        s_Instance->m_RenderStatistics.SpotLightCount += s_Instance->m_SpotLightDrawData.size();

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(4, descriptorHeap.GetGPUBaseDescriptor());

        for (auto& [meshHash, meshDrawData] : s_Instance->m_MeshDrawData)
        {
            auto& mesh = meshDrawData.Mesh;
            auto& meshInstanceData = meshDrawData.MeshInstanceData;

            auto vertexPositionsBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_POSITION);
            auto vertexTexCoordBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_TEX_COORD);
            auto vertexNormalBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_NORMAL);
            auto indexBuffer = mesh->GetBuffer(MeshBufferAttributeType::ATTRIB_INDEX);

            auto& instanceBuffer = s_Instance->m_MeshInstanceBuffers.at(meshHash);
            instanceBuffer->SetBufferData(&meshInstanceData[0], meshInstanceData.size() * sizeof(MeshInstanceData));

            commandList->SetVertexBuffers(0, 1, *vertexPositionsBuffer);
            commandList->SetVertexBuffers(1, 1, *vertexTexCoordBuffer);
            commandList->SetVertexBuffers(2, 1, *vertexNormalBuffer);
            commandList->SetVertexBuffers(3, 1, *instanceBuffer);
            commandList->SetIndexBuffer(*indexBuffer);

            commandList->DrawIndexed(indexBuffer->GetBufferDesc().NumElements, meshInstanceData.size());

            s_Instance->m_RenderStatistics.DrawCallCount++;
            s_Instance->m_RenderStatistics.TriangleCount += (indexBuffer->GetBufferDesc().NumElements / 3) * meshInstanceData.size();
            s_Instance->m_RenderStatistics.MeshCount += meshInstanceData.size();
        }

        RenderBackend::Get().ExecuteCommandList(commandList);
    }

    {
        /* Tone mapping render pass */
        auto commandList = RenderBackend::Get().GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        auto& tmColorTarget = s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
        auto& tmDepthBuffer = s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetDepthAttachment();

        auto tmRtv = tmColorTarget.GetRenderTargetDepthStencilView();
        auto tmDsv = tmDepthBuffer.GetRenderTargetDepthStencilView();

        // Set viewports, scissor rects and render targets
        commandList->SetViewports(1, &s_Instance->m_Viewport);
        commandList->SetScissorRects(1, &s_Instance->m_ScissorRect);
        commandList->SetRenderTargets(1, &tmRtv, &tmDsv);

        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, descriptorHeap);

        // Set pipeline state and root signature
        commandList->SetPipelineState(s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

        uint32_t pbrColorTargetIndex = s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment().GetSRVIndex();
        s_Instance->m_TonemapSettings.HDRTargetIndex = pbrColorTargetIndex;
        s_Instance->m_TonemapConstantBuffer->SetBufferData(&s_Instance->m_TonemapSettings);
        commandList->SetRootConstantBufferView(0, *s_Instance->m_TonemapConstantBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);

        // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
        commandList->SetRootDescriptorTable(1, descriptorHeap.GetGPUBaseDescriptor());

        // Set tone mapping vertex buffer
        commandList->SetVertexBuffers(0, 1, *s_Instance->m_ToneMapVertexBuffer.get());
        commandList->SetIndexBuffer(*s_Instance->m_ToneMapIndexBuffer.get());

        commandList->DrawIndexed(s_Instance->m_ToneMapIndexBuffer->GetBufferDesc().NumElements, 1);

        RenderBackend::Get().ExecuteCommandList(commandList);
    }
}

void Renderer::OnImGuiRender()
{
    ImGui::Text("Renderer");
    ImGui::Text("Resolution: %ux%u", s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", s_Instance->m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Draw calls: %u", s_Instance->m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", s_Instance->m_RenderStatistics.TriangleCount);
    ImGui::Text("Mesh count: %u", s_Instance->m_RenderStatistics.MeshCount);
    ImGui::Text("Directional light count: %u", s_Instance->m_RenderStatistics.DirectionalLightCount);
    ImGui::Text("Point light count: %u", s_Instance->m_RenderStatistics.PointLightCount);
    ImGui::Text("Spot light count: %u", s_Instance->m_RenderStatistics.SpotLightCount);
    ImGui::Text("Max model instances: %u", s_Instance->m_RenderSettings.MaxModelInstances);
    ImGui::Text("Max directional lights: %u", s_Instance->m_RenderSettings.MaxDirectionalLights);
    ImGui::Text("Max point lights: %u", s_Instance->m_RenderSettings.MaxPointLights);
    ImGui::Text("Max spot lights: %u", s_Instance->m_RenderSettings.MaxSpotLights);

    ImGui::Text("Tonemapping type");
    std::string previewValue = TonemapTypeToString(s_Instance->m_TonemapSettings.Type);
    if (ImGui::BeginCombo("##TonemapType", previewValue.c_str()))
    {
        for (uint32_t i = 0; i < static_cast<uint32_t>(TonemapType::NUM_TYPES); ++i)
        {
            std::string tonemapName = TonemapTypeToString(static_cast<TonemapType>(i)).c_str();
            bool isSelected = previewValue == tonemapName;

            if (ImGui::Selectable(tonemapName.c_str(), isSelected))
                s_Instance->m_TonemapSettings.Type = static_cast<TonemapType>(i);
            if (isSelected)
                ImGui::SetItemDefaultFocus();
        }

        ImGui::EndCombo();
    }
}

void Renderer::EndScene()
{
    SCOPED_TIMER("Renderer::EndScene");

    RenderBackend::Get().ResolveToBackBuffer(s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment());
    RenderBackend::Get().SwapBuffers(s_Instance->m_RenderSettings.VSync);

    s_Instance->m_MeshDrawData.clear();
    s_Instance->m_DirectionalLightDrawData.clear();
    s_Instance->m_PointLightDrawData.clear();
    s_Instance->m_SpotLightDrawData.clear();

    s_Instance->m_RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    uint32_t baseColorTexIndex = mesh->GetTexture(MeshTextureType::TEX_BASE_COLOR)->GetSRVIndex();
    uint32_t normalTexIndex = mesh->GetTexture(MeshTextureType::TEX_NORMAL)->GetSRVIndex();

    auto iter = s_Instance->m_MeshDrawData.find(mesh->GetHash());

    if (iter != s_Instance->m_MeshDrawData.end())
    {
        iter->second.MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f),
            baseColorTexIndex, normalTexIndex));
    }
    else
    {
        s_Instance->m_MeshDrawData.emplace(std::pair<std::size_t, MeshDrawData>(mesh->GetHash(), mesh));
        s_Instance->m_MeshDrawData.at(mesh->GetHash()).MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f),
            baseColorTexIndex, normalTexIndex));
    }

    auto instanceBufferIter = s_Instance->m_MeshInstanceBuffers.find(mesh->GetHash());

    if (instanceBufferIter == s_Instance->m_MeshInstanceBuffers.end())
    {
        s_Instance->m_MeshInstanceBuffers.emplace(std::pair<std::size_t, std::unique_ptr<Buffer>>(mesh->GetHash(),
            std::make_unique<Buffer>("Mesh instance buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1000, sizeof(MeshInstanceData)))));
    }
}

void Renderer::Submit(const DirectionalLightData& dirLightData)
{
    s_Instance->m_DirectionalLightDrawData.push_back(dirLightData);
    ASSERT(s_Instance->m_DirectionalLightDrawData.size() <= s_Instance->m_RenderSettings.MaxDirectionalLights, "Exceeded the maximum amount of directional lights");
}

void Renderer::Submit(const PointLightData& pointlightData)
{
    s_Instance->m_PointLightDrawData.push_back(pointlightData);
    ASSERT(s_Instance->m_PointLightDrawData.size() <= s_Instance->m_RenderSettings.MaxPointLights, "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const SpotLightData& spotLightData)
{
    s_Instance->m_SpotLightDrawData.push_back(spotLightData);
    ASSERT(s_Instance->m_SpotLightDrawData.size() <= s_Instance->m_RenderSettings.MaxSpotLights, "Exceeded the maximum amount of spot lights");
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (s_Instance->m_RenderSettings.Resolution.x != width || s_Instance->m_RenderSettings.Resolution.y != height)
    {
        s_Instance->m_RenderSettings.Resolution.x = std::max(1u, width);
        s_Instance->m_RenderSettings.Resolution.y = std::max(1u, height);

        s_Instance->m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(s_Instance->m_RenderSettings.Resolution.x),
            static_cast<float>(s_Instance->m_RenderSettings.Resolution.y), 0.0f, 1.0f);

        RenderBackend::Get().Resize(width, height);

        for (auto& renderPass : s_Instance->m_RenderPasses)
            renderPass->Resize(s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
    }
}

void Renderer::ToggleVSync()
{
    s_Instance->m_RenderSettings.VSync = !s_Instance->m_RenderSettings.VSync;
}

bool Renderer::IsVSyncEnabled()
{
    return s_Instance->m_RenderSettings.VSync;
}

const Renderer::RenderSettings& Renderer::GetSettings()
{
    return s_Instance->m_RenderSettings;
}

// Temp
Texture& Renderer::GetFinalColorOutput()
{
    return s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
}

// Temp
Texture& Renderer::GetFinalDepthOutput()
{
    return s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetDepthAttachment();
}

void Renderer::MakeRenderPasses()
{
    {
        // Default render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Default_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Default_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT,
            s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

        desc.RootParameters.resize(5);
        desc.RootParameters[0].InitAsConstantBufferView(0);
        desc.RootParameters[1].InitAsConstantBufferView(1);
        desc.RootParameters[2].InitAsConstantBufferView(2);
        desc.RootParameters[3].InitAsConstantBufferView(3);
        desc.RootParameters[4].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "TEX_INDICES", 0, DXGI_FORMAT_R32G32_UINT, 3, 80, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Instance->m_RenderPasses[RenderPassType::DEFAULT] = std::make_unique<RenderPass>("PBR", desc);
    }

    {
        // Tonemapping render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ToneMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ToneMapping_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            s_Instance->m_RenderSettings.Resolution.x, s_Instance->m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = false;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

        // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
        CD3DX12_DESCRIPTOR_RANGE1 ranges[1] = {};
        ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 256, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE, 0);

        desc.RootParameters.resize(2);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);
        desc.RootParameters[1].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING] = std::make_unique<RenderPass>("Tonemapping", desc);
    }
}

void Renderer::MakeBuffers()
{
    s_Instance->m_DirectionalLightBuffer = std::make_unique<Buffer>("Directional light constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Instance->m_RenderSettings.MaxDirectionalLights, sizeof(DirectionalLightData)));
    s_Instance->m_PointLightBuffer = std::make_unique<Buffer>("Pointlight constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Instance->m_RenderSettings.MaxPointLights, sizeof(PointLightData)));
    s_Instance->m_SpotLightBuffer = std::make_unique<Buffer>("Spotlight constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Instance->m_RenderSettings.MaxSpotLights, sizeof(SpotLightData)));

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
    s_Instance->m_TonemapConstantBuffer = std::make_unique<Buffer>("Tonemap constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(TonemapSettings)));
    s_Instance->m_ToneMapVertexBuffer = std::make_unique<Buffer>("Tonemap vertex buffer", BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, 4, sizeof(float) * 4), &toneMappingVertices[0]);
    s_Instance->m_ToneMapIndexBuffer = std::make_unique<Buffer>("Tonemap index buffer", BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, 6, sizeof(WORD)), &toneMappingIndices[0]);

    s_Instance->m_SceneDataConstantBuffer = std::make_unique<Buffer>("Scene data constant buffer", BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
}

std::string Renderer::TonemapTypeToString(TonemapType type)
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
