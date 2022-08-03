#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/Device.h"
#include "Graphics/SwapChain.h"
#include "Graphics/DescriptorHeap.h"
#include "Graphics/Buffer.h"
#include "Graphics/Shader.h"
#include "Resource/Model.h"
#include "Graphics/PipelineState.h"
#include "Graphics/RootSignature.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Renderer::Renderer()
{
}

Renderer::~Renderer()
{
}

void Renderer::Initialize(uint32_t width, uint32_t height)
{
    m_RenderSettings.Resolution.x = width;
    m_RenderSettings.Resolution.y = height;

    m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(m_RenderSettings.Resolution.x), static_cast<float>(m_RenderSettings.Resolution.y), 0.0f, 1.0f);
    m_ScissorRect = CD3DX12_RECT(0, 0, LONG_MAX, LONG_MAX);

    m_Device = std::make_shared<Device>();

    m_CommandQueueDirect = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
    m_CommandQueueCopy = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

    m_SwapChain = std::make_shared<SwapChain>(Application::Get().GetWindow()->GetHandle(), width, height);

    for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
        m_DescriptorHeaps[i] = std::make_unique<DescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i));

    PipelineStateDesc defaultPipelineDesc = {};
    defaultPipelineDesc.VertexShaderPath = L"Resources/Shaders/Default_VS.hlsl";
    defaultPipelineDesc.PixelShaderPath = L"Resources/Shaders/Default_PS.hlsl";
    defaultPipelineDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA16_UNORM,
        m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    defaultPipelineDesc.DepthStencilAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);

	std::vector<D3D12_INPUT_ELEMENT_DESC> defaultInputLayout = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 2, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 3, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

    std::vector<CD3DX12_DESCRIPTOR_RANGE1> descriptorRanges;
    descriptorRanges.resize(2);
    descriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 2);
    descriptorRanges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 2, 0);

    std::vector<CD3DX12_ROOT_PARAMETER1> rootParameters;
    rootParameters.resize(3);
    rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
    rootParameters[1].InitAsConstants(1, 1, 0, D3D12_SHADER_VISIBILITY_PIXEL);
    rootParameters[2].InitAsDescriptorTable(descriptorRanges.size(), &descriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    m_PipelineState[PipelineStateType::DEFAULT] = std::make_unique<PipelineState>(defaultPipelineDesc, defaultInputLayout, descriptorRanges, rootParameters);

    PipelineStateDesc toneMappingPipelineDesc = {};
    toneMappingPipelineDesc.VertexShaderPath = L"Resources/Shaders/ToneMapping_VS.hlsl";
    toneMappingPipelineDesc.PixelShaderPath = L"Resources/Shaders/ToneMapping_PS.hlsl";
    toneMappingPipelineDesc.ColorAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_RENDER_TARGET, TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM,
        m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    toneMappingPipelineDesc.DepthStencilAttachmentDesc = TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH, TextureFormat::TEXTURE_FORMAT_DEPTH32,
        m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    toneMappingPipelineDesc.DepthTest = false;

    std::vector<D3D12_INPUT_ELEMENT_DESC> toneMappingInputLayout = {
        { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    std::vector<CD3DX12_DESCRIPTOR_RANGE1> toneMappingDescriptorRanges;
    toneMappingDescriptorRanges.resize(1);
    toneMappingDescriptorRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

    std::vector<CD3DX12_ROOT_PARAMETER1> toneMappingRootParameters;
    toneMappingRootParameters.resize(1);
    toneMappingRootParameters[0].InitAsDescriptorTable(toneMappingDescriptorRanges.size(), &toneMappingDescriptorRanges[0], D3D12_SHADER_VISIBILITY_PIXEL);

    m_PipelineState[PipelineStateType::TONE_MAPPING] = std::make_unique<PipelineState>(toneMappingPipelineDesc, toneMappingInputLayout, toneMappingDescriptorRanges, toneMappingRootParameters);

    m_ModelInstanceBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_UPLOAD, m_RenderSettings.MaxModelInstances, sizeof(ModelInstanceData)));
    m_PointlightBuffer = std::make_unique<Buffer>(BufferDesc(BufferUsage::BUFFER_USAGE_CONSTANT, m_RenderSettings.MaxPointLights, sizeof(PointlightData)));

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
}

void Renderer::Finalize()
{
    Flush();
}

void Renderer::BeginFrame(const Camera& camera)
{
    SCOPED_TIMER("Renderer::BeginFrame");

    m_SceneData.Camera = camera;

    auto commandList = m_CommandQueueDirect->GetCommandList();

    for (uint32_t i = 0; i < PipelineStateType::NUM_PIPELINE_STATE_TYPES; ++i)
    {
        auto& colorTarget = m_PipelineState[i]->GetColorAttachment();
        auto& depthBuffer = m_PipelineState[i]->GetDepthStencilAttachment();

        auto rtv = colorTarget.GetDescriptorHandle();
        auto dsv = depthBuffer.GetDescriptorHandle();

        // Clear render target and depth stencil view
        float clearColor[4] = { 0.2f, 0.2f, 0.2f, 1.0f };
        commandList->ClearRenderTargetView(rtv, clearColor);
        commandList->ClearDepthStencilView(dsv);
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    /*
    
        Default pipeline
    
    */
    auto commandList = m_CommandQueueDirect->GetCommandList();
    auto& defaultColorTarget = m_PipelineState[PipelineStateType::DEFAULT]->GetColorAttachment();
    auto& defaultDepthBuffer = m_PipelineState[PipelineStateType::DEFAULT]->GetDepthStencilAttachment();

    auto rtv = defaultColorTarget.GetDescriptorHandle();
    auto dsv = defaultDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList->SetViewports(1, &m_Viewport);
    commandList->SetScissorRects(1, &m_ScissorRect);
    commandList->SetRenderTargets(1, &rtv, &dsv);

    // Set pipeline state, root signature, and root constants
    commandList->SetPipelineState(*m_PipelineState[PipelineStateType::DEFAULT].get());
    glm::mat4 viewProjection = m_SceneData.Camera.GetViewProjection();
    commandList->SetRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);
    uint32_t numPointlights = m_Pointlights.size();
    commandList->SetRoot32BitConstants(1, sizeof(uint32_t) / 4, &numPointlights, 0);

    // Set constant buffer data for point lights
    if (m_Pointlights.size() > 0)
    {
        m_PointlightBuffer->SetBufferData(&m_Pointlights[0], m_Pointlights.size() * sizeof(PointlightData));
    }

    commandList->SetConstantBufferView(2, 0, *m_PointlightBuffer.get(), D3D12_RESOURCE_STATE_GENERIC_READ);
    m_RenderStatistics.PointLightCount += m_Pointlights.size();

    for (auto& [modelName, modelDrawData] : m_ModelDrawData)
    {
        // Set mesh vertex buffers and index buffer
        commandList->SetVertexBuffers(0, 1, *modelDrawData.Model->GetBuffer(Model::InputType::INPUT_POSITION).get());
        commandList->SetVertexBuffers(1, 1, *modelDrawData.Model->GetBuffer(Model::InputType::INPUT_TEX_COORD).get());
        commandList->SetVertexBuffers(2, 1, *modelDrawData.Model->GetBuffer(Model::InputType::INPUT_NORMAL).get());
        commandList->SetIndexBuffer(*modelDrawData.Model->GetBuffer(Model::InputType::INPUT_INDEX).get());

        // Set instance buffer data for current mesh
        m_ModelInstanceBuffer->SetBufferData(&modelDrawData.ModelInstanceData[0], modelDrawData.ModelInstanceData.size() * sizeof(ModelInstanceData));
        commandList->SetVertexBuffers(3, 1, *m_ModelInstanceBuffer.get());

        // Set shader resource views for albedo and normal textures
        commandList->SetShaderResourceView(2, 1, *modelDrawData.Model->GetTexture(Model::TextureType::TEX_ALBEDO).get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->SetShaderResourceView(2, 2, *modelDrawData.Model->GetTexture(Model::TextureType::TEX_NORMAL).get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

        // Draw instanced/indexed model
        commandList->DrawIndexed(modelDrawData.Model->GetBuffer(Model::InputType::INPUT_INDEX)->GetBufferDesc().NumElements, modelDrawData.ModelInstanceData.size());

        // Transition albedo and normal textures back to common state from pixel shader resource state set in the SetShaderResourceView function
        CD3DX12_RESOURCE_BARRIER shaderResourceBarrier[2] = {
            CD3DX12_RESOURCE_BARRIER::Transition(modelDrawData.Model->GetTexture(Model::TextureType::TEX_ALBEDO)->GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON),
            CD3DX12_RESOURCE_BARRIER::Transition(modelDrawData.Model->GetTexture(Model::TextureType::TEX_NORMAL)->GetD3D12Resource().Get(),
            D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COMMON)
        };
        commandList->ResourceBarrier(2, shaderResourceBarrier);

        m_RenderStatistics.DrawCallCount++;
        m_RenderStatistics.TriangleCount += (modelDrawData.Model->GetBuffer(Model::InputType::INPUT_INDEX)->GetBufferDesc().NumElements / 3) * modelDrawData.ModelInstanceData.size();
        m_RenderStatistics.ObjectCount += modelDrawData.ModelInstanceData.size();
    }

    m_CommandQueueDirect->ExecuteCommandList(commandList);

    /*
    
        Post process: Tone mapping
    
    */
    auto commandList2 = m_CommandQueueDirect->GetCommandList();
    auto& tmColorTarget = m_PipelineState[PipelineStateType::TONE_MAPPING]->GetColorAttachment();
    auto& tmDepthBuffer = m_PipelineState[PipelineStateType::TONE_MAPPING]->GetDepthStencilAttachment();

    auto tmRtv = tmColorTarget.GetDescriptorHandle();
    auto tmDsv = tmDepthBuffer.GetDescriptorHandle();

    // Set viewports, scissor rects and render targets
    commandList2->SetViewports(1, &m_Viewport);
    commandList2->SetScissorRects(1, &m_ScissorRect);
    commandList2->SetRenderTargets(1, &tmRtv, &tmDsv);

    // Set pipeline state and root signature
    commandList2->SetPipelineState(*m_PipelineState[PipelineStateType::TONE_MAPPING].get());

    // Set tone mapping vertex buffer
    commandList2->SetVertexBuffers(0, 1, *m_ToneMapVertexBuffer.get());
    commandList2->SetIndexBuffer(*m_ToneMapIndexBuffer.get());

    // Set shader resource view to the previous color target
    auto& defaultColorTargetTexture = m_PipelineState[PipelineStateType::DEFAULT]->GetColorAttachment();
    CD3DX12_RESOURCE_BARRIER commonBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_COMMON);
    commandList2->ResourceBarrier(1, &commonBarrier);

    commandList2->SetShaderResourceView(0, 0, defaultColorTargetTexture, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
    commandList2->DrawIndexed(m_ToneMapIndexBuffer->GetBufferDesc().NumElements, 1);

    CD3DX12_RESOURCE_BARRIER rtBarrier = CD3DX12_RESOURCE_BARRIER::Transition(defaultColorTargetTexture.GetD3D12Resource().Get(),
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_RENDER_TARGET);
    commandList2->ResourceBarrier(1, &rtBarrier);

    m_CommandQueueDirect->ExecuteCommandList(commandList2);
}

void Renderer::ImGuiRender()
{
    ImGui::Begin("Renderer");

    ImGui::Text("Resolution: %ux%u", m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
    ImGui::Text("VSync: %s", m_RenderSettings.VSync ? "On" : "Off");
    ImGui::Text("Draw calls: %u", m_RenderStatistics.DrawCallCount);
    ImGui::Text("Triangle count: %u", m_RenderStatistics.TriangleCount);
    ImGui::Text("Object count: %u", m_RenderStatistics.ObjectCount);
    ImGui::Text("Point light count: %u", m_RenderStatistics.PointLightCount);
    ImGui::Text("Max model instances: %u", m_RenderSettings.MaxModelInstances);
    ImGui::Text("Max point lights: %u", m_RenderSettings.MaxPointLights);

    ImGui::End();
}

void Renderer::EndFrame()
{
    SCOPED_TIMER("Renderer::EndFrame");

    m_SwapChain->ResolveToBackBuffer(m_PipelineState[PipelineStateType::TONE_MAPPING]->GetColorAttachment());
    m_SwapChain->SwapBuffers();

    m_CommandQueueDirect->ResetCommandLists();
    m_CommandQueueCopy->ResetCommandLists();

    m_ModelDrawData.clear();
    m_Pointlights.clear();

    m_RenderStatistics.Reset();
}

void Renderer::Submit(const std::shared_ptr<Model>& model, const glm::mat4& transform)
{
    auto iter = m_ModelDrawData.find(model->GetName().c_str());
    if (iter != m_ModelDrawData.end())
    {
        iter->second.ModelInstanceData.emplace_back(transform, glm::vec4(1.0f));
    }
    else
    {
        m_ModelDrawData.emplace(std::pair<const char*, ModelDrawData>(model->GetName().c_str(), ModelDrawData(model)));
        m_ModelDrawData.at(model->GetName().c_str()).ModelInstanceData.emplace_back(transform, glm::vec4(1.0f));
    }

    ASSERT((m_ModelDrawData.size() <= m_RenderSettings.MaxModelInstances), "Exceeded the maximum amount of model instances");
}

void Renderer::Submit(const PointlightData& pointlightData)
{
    m_Pointlights.push_back(pointlightData);
    ASSERT((m_Pointlights.size() <= m_RenderSettings.MaxPointLights), "Exceeded the maximum amount of point lights");
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

        for (uint32_t i = 0; i < PipelineStateType::NUM_PIPELINE_STATE_TYPES; ++i)
        {
            m_PipelineState[i]->GetColorAttachment().Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
            m_PipelineState[i]->GetDepthStencilAttachment().Resize(m_RenderSettings.Resolution.x, m_RenderSettings.Resolution.y);
        }
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

void Renderer::Flush()
{
    m_CommandQueueDirect->Flush();
}
