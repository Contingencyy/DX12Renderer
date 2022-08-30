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
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

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
    SCOPED_TIMER("Renderer::BeginFrame");
    
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

        // Set pipeline state, root signature, and scene data constant buffer
        commandList->SetPipelineState(s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetPipelineState());

        s_Instance->m_SceneData.NumPointlights = s_Instance->m_PointlightDrawData.size();

        s_Instance->m_SceneDataConstantBuffer->SetBufferData(&s_Instance->m_SceneData);
        commandList->SetRootConstantBufferView(0, *s_Instance->m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set constant buffer data for point lights
        if (s_Instance->m_SceneData.NumPointlights > 0)
        {
            s_Instance->m_PointlightBuffer->SetBufferData(&s_Instance->m_PointlightDrawData[0], s_Instance->m_PointlightDrawData.size() * sizeof(PointlightData));
        }

        commandList->SetConstantBufferView(1, 0, *s_Instance->m_PointlightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
        s_Instance->m_RenderStatistics.PointLightCount += s_Instance->m_PointlightDrawData.size();

        for (auto& [meshHash, meshDrawData] : s_Instance->m_MeshDrawData)
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

            s_Instance->m_MeshInstanceBuffer->SetBufferData(&meshInstanceData[0], meshInstanceData.size() * sizeof(MeshInstanceData));
            commandList->SetVertexBuffers(3, 1, *s_Instance->m_MeshInstanceBuffer);

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

        // Set pipeline state and root signature
        commandList->SetPipelineState(s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

        s_Instance->m_TonemapConstantBuffer->SetBufferData(&s_Instance->m_TonemapSettings);
        commandList->SetRootConstantBufferView(0, *s_Instance->m_TonemapConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

        // Set tone mapping vertex buffer
        commandList->SetVertexBuffers(0, 1, *s_Instance->m_ToneMapVertexBuffer.get());
        commandList->SetIndexBuffer(*s_Instance->m_ToneMapIndexBuffer.get());

        // Set shader resource view to the previous color target
        auto& defaultColorTargetTexture = s_Instance->m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
        CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
        commandList->ResourceBarrier(1, &commonBarrier);

        commandList->SetShaderResourceView(1, 0, defaultColorTargetTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->DrawIndexed(s_Instance->m_ToneMapIndexBuffer->GetBufferDesc().NumElements, 1);

        CD3DX12_RESOURCE_BARRIER rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &rtBarrier);

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
    ImGui::Text("Point light count: %u", s_Instance->m_RenderStatistics.PointLightCount);
    ImGui::Text("Max model instances: %u", s_Instance->m_RenderSettings.MaxModelInstances);
    ImGui::Text("Max point lights: %u", s_Instance->m_RenderSettings.MaxPointLights);

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
    SCOPED_TIMER("Renderer::EndFrame");

    RenderBackend::Get().ResolveToBackBuffer(s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment());
    RenderBackend::Get().SwapBuffers(s_Instance->m_RenderSettings.VSync);

    s_Instance->m_MeshDrawData.clear();
    s_Instance->m_PointlightDrawData.clear();

    s_Instance->m_RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    auto iter = s_Instance->m_MeshDrawData.find(mesh->GetHash());
    if (iter != s_Instance->m_MeshDrawData.end())
    {
        iter->second.MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
    else
    {
        s_Instance->m_MeshDrawData.emplace(std::pair<std::size_t, MeshDrawData>(mesh->GetHash(), mesh));
        s_Instance->m_MeshDrawData.at(mesh->GetHash()).MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
}

void Renderer::Submit(const PointlightData& pointlightData)
{
    s_Instance->m_PointlightDrawData.push_back(pointlightData);
    ASSERT((s_Instance->m_PointlightDrawData.size() <= s_Instance->m_RenderSettings.MaxPointLights), "Exceeded the maximum amount of point lights");
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

        desc.DescriptorRanges.resize(2);
        desc.DescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1);
        desc.DescriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

        desc.RootParameters.resize(2);
        desc.RootParameters[0].InitAsConstantBufferView(0);
        desc.RootParameters[1].InitAsDescriptorTable(desc.DescriptorRanges.size(), &desc.DescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 });

        s_Instance->m_RenderPasses[RenderPassType::DEFAULT] = std::make_unique<RenderPass>(desc);
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

        desc.DescriptorRanges.resize(1);
        desc.DescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

        desc.RootParameters.resize(2);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE, D3D12_SHADER_VISIBILITY_PIXEL);
        desc.RootParameters[1].InitAsDescriptorTable(desc.DescriptorRanges.size(), &desc.DescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        s_Instance->m_RenderPasses[RenderPassType::TONE_MAPPING] = std::make_unique<RenderPass>(desc);
    }
}

void Renderer::MakeBuffers()
{
    s_Instance->m_MeshInstanceBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, s_Instance->m_RenderSettings.MaxModelInstances, sizeof(MeshInstanceData)));
    s_Instance->m_PointlightBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, s_Instance->m_RenderSettings.MaxPointLights, sizeof(PointlightData)));

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
    s_Instance->m_TonemapConstantBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(TonemapSettings)));
    s_Instance->m_ToneMapVertexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, 4, sizeof(float) * 4), &toneMappingVertices[0]);
    s_Instance->m_ToneMapIndexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, 6, sizeof(WORD)), &toneMappingIndices[0]);

    s_Instance->m_SceneDataConstantBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
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
