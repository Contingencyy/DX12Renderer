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

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    m_RenderSettings.Resolution.x = width;
    m_RenderSettings.Resolution.y = height;

    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x), static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);
    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    m_Device = std::make_shared<Device>();

    m_CommandQueueDirect = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueueCopy = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

    m_SwapChain = std::make_shared<SwapChain>(hWnd, width, height);

    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_DescriptorHeaps[i] = std::make_unique<DescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

    CreateRenderPasses();
    InitializeBuffers();
}

void Renderer::Finalize()
{
    Flush();
}

void Renderer::BeginFrame(const Camera& camera)
{
    SCOPED_TIMER("Renderer::BeginFrame");

    m_SceneData.ViewProjection = camera.GetViewProjection();

    auto commandList = m_CommandQueueDirect->GetCommandList();

    for (auto& renderPass : m_RenderPasses)
        renderPass->ClearViews(commandList);

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    /*
    
        Default pipeline
    
    */
    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto& defaultColorTarget = m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
    auto& defaultDepthBuffer = m_RenderPasses[RenderPassType::DEFAULT]->GetDepthAttachment();

    auto rtv = defaultColorTarget.GetDescriptorHandle();
    auto dsv = defaultDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList->SetViewports(1, &m_Viewport);
    commandList->SetScissorRects(1, &m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set pipeline state, root signature, and scene data constant buffer
    commandList->SetPipelineState(m_RenderPasses[RenderPassType::DEFAULT]->GetPipelineState());

    m_SceneData.Ambient = glm::vec3(0.1f);
    m_SceneData.NumPointlights = m_PointlightDrawData.size();

    m_SceneDataConstantBuffer->SetBufferData(&m_SceneData);
    commandList->SetRootConstantBufferView(0, *m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

    // Set constant buffer data for point lights
    if (m_SceneData.NumPointlights > 0)
    {
        m_PointlightBuffer->SetBufferData(&m_PointlightDrawData[0], m_PointlightDrawData.size() * sizeof(PointlightData));
    }

    commandList->SetConstantBufferView(1, 0, *m_PointlightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    m_RenderStatistics.PointLightCount += m_PointlightDrawData.size();

    for (auto& [meshHash, meshDrawData] : m_MeshDrawData)
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

        m_MeshInstanceBuffer->SetBufferData(&meshInstanceData[0], meshInstanceData.size() * sizeof(MeshInstanceData));
        commandList->SetVertexBuffers(3, 1, *m_MeshInstanceBuffer);

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

        m_RenderStatistics.DrawCallCount++;
        m_RenderStatistics.TriangleCount += (indexBuffer->GetBufferDesc().NumElements / 3) * meshInstanceData.size();
        m_RenderStatistics.MeshCount += meshInstanceData.size();
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);

    /*
    
        Post process: Tone mapping
    
    */
    auto commandList2 = m_CommandQueueDirect->GetCommandList();
    auto& tmColorTarget = m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment();
    auto& tmDepthBuffer = m_RenderPasses[RenderPassType::TONE_MAPPING]->GetDepthAttachment();

    auto tmRtv = tmColorTarget.GetDescriptorHandle();
    auto tmDsv = tmDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList2->SetViewports(1, &m_Viewport);
    commandList2->SetScissorRects(1, &m_ScissorRect);
    commandList2->SetRenderTargets(1, &tmRtv, &tmDsv);

    // Set pipeline state and root signature
    commandList2->SetPipelineState(m_RenderPasses[RenderPassType::TONE_MAPPING]->GetPipelineState());

    // Set tone mapping vertex buffer
    commandList2->SetVertexBuffers(0, 1, *m_ToneMapVertexBuffer.get());
    commandList2->SetIndexBuffer(*m_ToneMapIndexBuffer.get());

    // Set shader resource view to the previous color target
    auto& defaultColorTargetTexture = m_RenderPasses[RenderPassType::DEFAULT]->GetColorAttachment();
    CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
    commandList2->ResourceBarrier(1, &commonBarrier);

    commandList2->SetShaderResourceView(0, 0, defaultColorTargetTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList2->DrawIndexed(m_ToneMapIndexBuffer->GetBufferDesc().NumElements, 1);

    CD3DX12_RESOURCE_BARRIER rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList2->ResourceBarrier(1, &rtBarrier);

    m_CommandQueueDirect->ExecuteCommandList(commandList2);

    /*

        Debug line

    */

    if (m_LineVertexData.size() == 0)
        return;

    auto commandList3 = m_CommandQueueDirect->GetCommandList();
    auto& dlColorTarget = m_RenderPasses[RenderPassType::DEBUG_LINE]->GetColorAttachment();
    auto& dlDepthBuffer = m_RenderPasses[RenderPassType::DEBUG_LINE]->GetDepthAttachment();

    auto dlRtv = dlColorTarget.GetDescriptorHandle();
    auto dlDsv = dlDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList3->SetViewports(1, &m_Viewport);
    commandList3->SetScissorRects(1, &m_ScissorRect);
    commandList3->SetRenderTargets(1, &tmRtv, &dsv);

    // Set pipeline state and root signature
    commandList3->SetPipelineState(m_RenderPasses[RenderPassType::DEBUG_LINE]->GetPipelineState());
    commandList3->SetRootConstantBufferView(0, *m_SceneDataConstantBuffer.get(), D3D12_RESOURCE_STATE_COMMON);

    m_LineBuffer->SetBufferData(&m_LineVertexData[0], sizeof(LineVertex) * m_LineVertexData.size());
    commandList3->SetVertexBuffers(0, 1, *m_LineBuffer);

    commandList3->Draw(m_LineVertexData.size(), 1);

    m_CommandQueueDirect->ExecuteCommandList(commandList3);
}

void Renderer::ImGuiRender()
{
    ImGui::Begin("Renderer");

    ImGui::Text("Resolution: %ux%u", m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Draw calls: %u", m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", m_RenderStatistics.TriangleCount);
    ImGui::Text("Mesh count: %u", m_RenderStatistics.MeshCount);
    ImGui::Text("Point light count: %u", m_RenderStatistics.PointLightCount);
    ImGui::Text("Max model instances: %u", m_RenderSettings.MaxModelInstances);
    ImGui::Text("Max point lights: %u", m_RenderSettings.MaxPointLights);

    ImGui::End();
}

void Renderer::EndFrame()
{
    SCOPED_TIMER("Renderer::EndFrame");

    m_SwapChain->ResolveToBackBuffer(m_RenderPasses[RenderPassType::TONE_MAPPING]->GetColorAttachment());
    m_SwapChain->SwapBuffers();

    m_CommandQueueDirect->ResetCommandLists();
    m_CommandQueueCopy->ResetCommandLists();

    m_MeshDrawData.clear();
    m_PointlightDrawData.clear();
    m_LineVertexData.clear();

    m_RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform)
{
    auto iter = m_MeshDrawData.find(mesh->GetHash());
    if (iter != m_MeshDrawData.end())
    {
        iter->second.MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
    else
    {
        m_MeshDrawData.emplace(std::pair<std::size_t, MeshDrawData>(mesh->GetHash(), mesh));
        m_MeshDrawData.at(mesh->GetHash()).MeshInstanceData.push_back(MeshInstanceData(transform, glm::vec4(1.0f)));
    }
}

void Renderer::Submit(const PointlightData& pointlightData)
{
    m_PointlightDrawData.push_back(pointlightData);
    ASSERT((m_PointlightDrawData.size() <= m_RenderSettings.MaxPointLights), "Exceeded the maximum amount of point lights");
}

void Renderer::Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color)
{
    m_LineVertexData.emplace_back(lineStart, color);
    m_LineVertexData.emplace_back(lineEnd, color);
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (m_RenderSettings.Resolution.x != width || m_RenderSettings.Resolution.y != height)
    {
        m_RenderSettings.Resolution.x = std::max(1u, width);
        m_RenderSettings.Resolution.y = std::max(1u, height);

        m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x),
            static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);

        Flush();

        m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->Reset();
        m_SwapChain->Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);

        for (auto& renderPass : m_RenderPasses)
            renderPass->Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    }
}

void Renderer::CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyBuffer(intermediateBuffer, destBuffer, bufferData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void Renderer::CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData)
{
    auto commandList = m_CommandQueueCopy->GetCommandList();
    commandList->CopyTexture(intermediateBuffer, destTexture, textureData);

    uint64_t fenceValue = m_CommandQueueCopy->ExecuteCommandList(commandList);
    m_CommandQueueCopy->WaitForFenceValue(fenceValue);
}

DescriptorAllocation Renderer::AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return m_DescriptorHeaps[type]->Allocate(numDescriptors);
}

// Temp
RenderPass& Renderer::GetRenderPass() const
{
    return *m_RenderPasses[RenderPassType::TONE_MAPPING];
}

void Renderer::CreateRenderPasses()
{
    {
        // Default render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/Default_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/Default_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA16_UNORM,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
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

        m_RenderPasses[RenderPassType::DEFAULT] = std::make_unique<RenderPass>(desc);
    }

    {
        // Debug line render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/DebugLine_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/DebugLine_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
        desc.ClearColor = glm::vec4(0.2f, 0.2f, 0.2f, 1.0f);
        desc.DepthEnabled = true;
        desc.Topology = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
        desc.TopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;

        desc.RootParameters.resize(1);
        desc.RootParameters[0].InitAsConstantBufferView(0, 0);

        desc.ShaderInputLayout.push_back({ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });
        desc.ShaderInputLayout.push_back({ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 });

        m_RenderPasses[RenderPassType::DEBUG_LINE] = std::make_unique<RenderPass>(desc);
    }

    {
        // Tonemapping render pass
        RenderPassDesc desc;
        desc.VertexShaderPath = "Resources/Shaders/ToneMapping_VS.hlsl";
        desc.PixelShaderPath = "Resources/Shaders/ToneMapping_PS.hlsl";
        desc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
        desc.DepthAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
            m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
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

        m_RenderPasses[RenderPassType::TONE_MAPPING] = std::make_unique<RenderPass>(desc);
    }
}

void Renderer::InitializeBuffers()
{
    m_MeshInstanceBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, m_RenderSettings.MaxModelInstances, sizeof(MeshInstanceData)));
    m_PointlightBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, m_RenderSettings.MaxPointLights, sizeof(PointlightData)));
    m_LineBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, 10000, sizeof(LineVertex)));

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
    m_ToneMapVertexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_VERTEX, 4, sizeof(float) * 4), &toneMappingVertices[0]);
    m_ToneMapIndexBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_INDEX, 6, sizeof(WORD)), &toneMappingIndices[0]);

    m_SceneDataConstantBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, 1, sizeof(SceneData)));
}

void Renderer::Flush()
{
    m_CommandQueueDirect->Flush();
}
