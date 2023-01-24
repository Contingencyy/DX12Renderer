#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderState.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/ResourceSlotmap.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Shader.h"
#include "Graphics/RasterPass.h"
#include "Graphics/ComputePass.h"
#include "Components/DirLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"

#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/RenderBackend.h"

#include <imgui/imgui.h>

struct SceneData
{
    void Reset()
    {
        DirLightCount = 0;
        PointLightCount = 0;
        SpotLightCount = 0;
    }

    glm::mat4 SceneCameraViewProjection = glm::identity<glm::mat4>();
    glm::vec3 SceneCameraPosition = glm::vec3(0.0f);

    uint32_t DirLightCount = 0;
    uint32_t PointLightCount = 0;
    uint32_t SpotLightCount = 0;
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
    uint32_t AlbedoTextureIndex = 0;
    uint32_t NormalTextureIndex = 0;
    uint32_t MetallicRoughnessTextureIndex = 0;
    float Metalness = 0.0f;
    float Roughness = 0.3f;
    BYTE_PADDING(12);
};;

struct MeshInstanceData
{
    glm::mat4 Transform = glm::identity<glm::mat4>();
    uint32_t MaterialID = 0;
};

struct MeshSubmission
{
    Mesh* Mesh;
    MeshInstanceData InstanceData;
};

struct LightSubmission
{
    Texture* ShadowMap;
    uint32_t Face;
    Camera LightCamera;
};

enum class TonemapType : uint32_t
{
    LINEAR, REINHARD, UNCHARTED2, FILMIC, ACES_FILMIC,
    NUM_TYPES
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

    // Tone mapping settings and buffers
    TonemapSettings TonemapSettings;

    // Scene data and buffer
    Camera SceneCamera;
    SceneData SceneData;

    // Trackers for current mesh, mesh instance, and material count
    std::size_t OpaqueMeshCount = 0;
    std::size_t TransparentMeshCount = 0;
    std::size_t MaterialCount = 0;
    std::size_t LightCount = 0;

    std::array<MeshSubmission, RenderState::MAX_MESH_INSTANCES> OpaqueMeshSubmissions;
    std::array<MeshSubmission, RenderState::MAX_MESH_INSTANCES> TransparentMeshSubmissions;
    std::array<LightSubmission, RenderState::MAX_DIR_LIGHTS * RenderState::MAX_SPOT_LIGHTS * (RenderState::MAX_POINT_LIGHTS * 6)> LightSubmissions;
};

static InternalRendererData s_Data;

namespace Renderer
{

    void CreateRenderPasses()
    {
        {
            // Shadow mapping render pass
            RasterPassDesc desc;
            desc.VertexShaderPath = "Resources/Shaders/ShadowMapping_VS.hlsl";
            desc.PixelShaderPath = "Resources/Shaders/ShadowMapping_PS.hlsl";

            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.ShadowMapResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.ShadowMapResolution.y;

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

            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;

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

            desc.ColorAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.ColorAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT;
            desc.ColorAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.ColorAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;

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
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

            desc.RootParameters.resize(3);
            desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // Tonemapping settings
            desc.RootParameters[1].InitAsDescriptorTable(1, &ranges[0]); // Bindless SRV table
            desc.RootParameters[2].InitAsDescriptorTable(1, &ranges[1]); // Bindless UAV table

            s_Data.ComputePasses[ComputePassType::TONE_MAPPING] = std::make_unique<ComputePass>("Tonemapping", desc);
        }
    }

    void CreateBuffers()
    {
        // TODO: Constant buffers should be replaced by a giant upload/constant buffer which can be suballocated from
        
        // Mesh instance buffers (opaque and transparent meshes)
        BufferDesc meshBufferDesc = {};
        meshBufferDesc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
        //meshBufferDesc.NumElements = g_RenderState.MAX_MESH_INSTANCES * g_RenderState.BACK_BUFFER_COUNT;
        meshBufferDesc.NumElements = g_RenderState.MAX_MESH_INSTANCES;
        meshBufferDesc.ElementSize = sizeof(MeshInstanceData);
        meshBufferDesc.DebugName = "Mesh instance buffer opaque";
        g_RenderState.MeshInstanceBufferOpaque = std::make_unique<Buffer>(meshBufferDesc);

        meshBufferDesc.DebugName = "Mesh instance buffer transparent";
        g_RenderState.MeshInstanceBufferTransparent = std::make_unique<Buffer>(meshBufferDesc);

        // Material constant buffer
        BufferDesc materialBufferDesc = {};
        materialBufferDesc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
        //materialBufferDesc.NumElements = g_RenderState.MAX_MATERIALS * g_RenderState.BACK_BUFFER_COUNT;
        materialBufferDesc.NumElements = g_RenderState.MAX_MATERIALS;
        materialBufferDesc.ElementSize = sizeof(MaterialData);
        materialBufferDesc.DebugName = "Material constant buffer";

        g_RenderState.MaterialConstantBuffer = std::make_unique<Buffer>(materialBufferDesc);

        // Light constant buffer
        BufferDesc lightBufferDesc = {};
        lightBufferDesc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
        lightBufferDesc.NumElements = g_RenderState.MAX_DIR_LIGHTS * sizeof(DirectionalLightData) +
            g_RenderState.MAX_SPOT_LIGHTS * sizeof(SpotLightData) + g_RenderState.MAX_POINT_LIGHTS * sizeof(PointLightData);
        lightBufferDesc.ElementSize = 1 * g_RenderState.BACK_BUFFER_COUNT;
        lightBufferDesc.DebugName = "Light constant buffer";

        g_RenderState.LightConstantBuffer = std::make_unique<Buffer>(lightBufferDesc);

        // Tonemap settings constant buffer
        BufferDesc tonemapBufferDesc = {};
        tonemapBufferDesc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
        tonemapBufferDesc.NumElements = 1 * g_RenderState.BACK_BUFFER_COUNT;
        tonemapBufferDesc.ElementSize = sizeof(TonemapSettings);
        tonemapBufferDesc.DebugName = "Tonemap settings constant buffer";

        g_RenderState.TonemapConstantBuffer = std::make_unique<Buffer>(tonemapBufferDesc);

        // Scene data constant buffer
        BufferDesc sceneDataBufferDesc = {};
        sceneDataBufferDesc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
        sceneDataBufferDesc.NumElements = 1 * g_RenderState.BACK_BUFFER_COUNT;
        sceneDataBufferDesc.ElementSize = sizeof(SceneData);
        sceneDataBufferDesc.DebugName = "Scene data constant buffer";

        g_RenderState.SceneDataConstantBuffer = std::make_unique<Buffer>(sceneDataBufferDesc);
    }

    void CreateRenderTargets()
    {
        // Depth pre-pass depth target
        TextureDesc depthPrepassDesc = {};
        depthPrepassDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
        depthPrepassDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
        depthPrepassDesc.Width = g_RenderState.Settings.RenderResolution.x;
        depthPrepassDesc.Height = g_RenderState.Settings.RenderResolution.y;
        depthPrepassDesc.ClearColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
        depthPrepassDesc.DebugName = "Depth pre-pass depth target";

        g_RenderState.DepthPrepassDepthTarget = std::make_unique<Texture>(depthPrepassDesc);

        // HDR color target
        TextureDesc hdrColorDesc = {};
        hdrColorDesc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
        hdrColorDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT;
        hdrColorDesc.Width = g_RenderState.Settings.RenderResolution.x;
        hdrColorDesc.Height = g_RenderState.Settings.RenderResolution.y;
        hdrColorDesc.DebugName = "HDR color target";

        g_RenderState.HDRColorTarget = std::make_unique<Texture>(hdrColorDesc);

        // SDR color target
        TextureDesc sdrColorDesc = {};
        sdrColorDesc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_WRITE;
        sdrColorDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
        sdrColorDesc.Width = g_RenderState.Settings.RenderResolution.x;
        sdrColorDesc.Height = g_RenderState.Settings.RenderResolution.y;
        sdrColorDesc.DebugName = "SDR color target";

        g_RenderState.SDRColorTarget = std::make_unique<Texture>(sdrColorDesc);
    }

    void CreateDefaultTextures()
    {
        // Make default white and normal textures
        uint32_t defaultWhiteTextureData = 0xFFFFFFFF;

        TextureDesc defaultTextureDesc = {};
        defaultTextureDesc.Usage = TextureUsage::TEXTURE_USAGE_READ;
        defaultTextureDesc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
        defaultTextureDesc.Width = 1;
        defaultTextureDesc.Height = 1;
        defaultTextureDesc.DataPtr = &defaultWhiteTextureData;
        defaultTextureDesc.DebugName = "Default white texture";

        g_RenderState.DefaultWhiteTexture = std::make_unique<Texture>(defaultTextureDesc);

        // Texture data is in ABGR layout, the default normal will point forward in tangent space (blue)
        uint32_t defaultNormalTextureData = (255 << 24) + (255 << 16) + (0 << 8) + 0;
        defaultTextureDesc.DataPtr = &defaultNormalTextureData;
        defaultTextureDesc.DebugName = "Default normal texture";

        g_RenderState.DefaultNormalTexture = std::make_unique<Texture>(defaultTextureDesc);
    }

    void ClearRenderTargets()
    {
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->Transition(*g_RenderState.HDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->Transition(*g_RenderState.SDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ClearDepthStencilView(g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV), g_RenderState.DepthPrepassDepthTarget->GetTextureDesc().ClearColor.x);
        commandList->ClearRenderTargetView(g_RenderState.HDRColorTarget->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(g_RenderState.HDRColorTarget->GetTextureDesc().ClearColor));
        commandList->ClearRenderTargetView(g_RenderState.SDRColorTarget->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(g_RenderState.SDRColorTarget->GetTextureDesc().ClearColor));

        RenderBackend::ExecuteCommandList(commandList);
    }

    void ResizeResolutionDependentResources(uint32_t width, uint32_t height)
    {
        g_RenderState.DepthPrepassDepthTarget->Resize(width, height);
        g_RenderState.HDRColorTarget->Resize(width, height);
        g_RenderState.SDRColorTarget->Resize(width, height);
    }

    void RenderGeometry(CommandList& commandList, const Camera& camera, TransparencyMode transparency)
    {
        std::array<MeshSubmission, RenderState::MAX_MESH_INSTANCES>* meshSubmissions;
        std::size_t numSubmissions;

        if (transparency == TransparencyMode::OPAQUE)
        {
            commandList.SetVertexBuffers(1, 1, *g_RenderState.MeshInstanceBufferOpaque);
            meshSubmissions = &s_Data.OpaqueMeshSubmissions;
            numSubmissions = s_Data.OpaqueMeshCount;
        }
        else
        {
            commandList.SetVertexBuffers(1, 1, *g_RenderState.MeshInstanceBufferTransparent);
            meshSubmissions = &s_Data.TransparentMeshSubmissions;
            numSubmissions = s_Data.TransparentMeshCount;
        }

        uint32_t currentInstance = 0;
        //RenderResourceHandle prevVertexBufferHandle;

        for (std::size_t m = 0; m < numSubmissions; ++m)
        {
            auto mesh = (*meshSubmissions)[m].Mesh;

            // TODO: Here we should transform the bounding box with this current instance's transform matrix

            if (camera.IsFrustumCullingEnabled())
            {
                if (!camera.GetViewFrustum().IsBoxInViewFrustum(mesh->BB.Min, mesh->BB.Max))
                {
                    currentInstance++;
                    continue;
                }
            }

            auto vertexBuffer = g_RenderState.BufferSlotmap.Find(mesh->VertexBuffer);
            auto indexBuffer = g_RenderState.BufferSlotmap.Find(mesh->IndexBuffer);

            commandList.SetVertexBuffers(0, 1, *vertexBuffer);
            commandList.SetIndexBuffer(*indexBuffer);

            /*if (RENDER_RESOURCE_HANDLE_COMPARE(prevVertexBufferHandle, mesh->VertexBuffer))
            {
                prevVertexBufferHandle = mesh->VertexBuffer;

                commandList.SetVertexBuffers(0, 1, *vertexBuffer);
                commandList.SetIndexBuffer(*indexBuffer);
            }*/

            commandList.DrawIndexed(static_cast<uint32_t>(indexBuffer->GetBufferDesc().NumElements), 1, 0, 0, currentInstance);

            g_RenderState.Stats.DrawCallCount++;
            currentInstance++;
        }
    }

    void RenderShadowMap(CommandList& commandList, const Camera& lightCamera, const Texture& shadowMap, uint32_t descriptorOffset)
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

}

void Renderer::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
    RenderBackend::Initialize(hWnd, width, height);

    g_RenderState.Settings.RenderResolution.x = width;
    g_RenderState.Settings.RenderResolution.y = height;

    CreateRenderPasses();
    CreateRenderTargets();
    CreateBuffers();
    CreateDefaultTextures();
}

void Renderer::Finalize()
{
    RenderBackend::Finalize();
}

void Renderer::BeginFrame()
{
    SCOPED_TIMER("Renderer::BeginFrame");

    RenderBackend::BeginFrame();
    ClearRenderTargets();
}

void Renderer::BeginScene(const Camera& sceneCamera)
{
    SCOPED_TIMER("Renderer::BeginScene");

    s_Data.SceneCamera = sceneCamera;
    s_Data.SceneData.SceneCameraViewProjection = sceneCamera.GetViewProjection();
    s_Data.SceneData.SceneCameraPosition = sceneCamera.GetTransform().GetPosition();

    s_Data.TonemapSettings.Exposure = sceneCamera.GetExposure();
    s_Data.TonemapSettings.Gamma = sceneCamera.GetGamma();
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    g_RenderState.SceneDataConstantBuffer->SetBufferData(&s_Data.SceneData);
    g_RenderState.Stats.MeshCount = s_Data.OpaqueMeshCount + s_Data.TransparentMeshCount;

    auto& bindlessDescriptorHeap = RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

    {
        /* Shadow mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Shadow mapping");

        // Clear all shadow map textures and transition them to the depth write state
        for (std::size_t i = 0; i < s_Data.LightCount; ++i)
        {
            commandList->Transition(*s_Data.LightSubmissions[i].ShadowMap, D3D12_RESOURCE_STATE_DEPTH_WRITE);
            commandList->ClearDepthStencilView(s_Data.LightSubmissions[i].ShadowMap->GetDescriptor(DescriptorType::DSV), 0.0f);
        }

        // Set bindless descriptor heap
        commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, bindlessDescriptorHeap);

        // Bind render pass bindables (sets viewport, scissor rect, render targets, pipeline state, root signature and primitive topology)
        commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::SHADOW_MAPPING]);

        for (std::size_t i = 0; i < s_Data.LightCount; ++i)
        {
            RenderShadowMap(*commandList, s_Data.LightSubmissions[i].LightCamera, *s_Data.LightSubmissions[i].ShadowMap, s_Data.LightSubmissions[i].Face);
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
            commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            // Bind render pass bindables
            commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::DEPTH_PREPASS]);
            
            CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(g_RenderState.Settings.RenderResolution.x),
                static_cast<float>(g_RenderState.Settings.RenderResolution.y), 0.0f, 1.0f);
            CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

            commandList->SetViewports(1, &viewport);
            commandList->SetScissorRects(1, &scissorRect);

            D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV);
            commandList->SetRenderTargets(0, nullptr, &dsv);

            commandList->SetRootConstantBufferView(0, *g_RenderState.SceneDataConstantBuffer, D3D12_RESOURCE_STATE_COMMON);

            RenderGeometry(*commandList, s_Data.SceneCamera, static_cast<TransparencyMode>(i));

            commandList->EndTimestampQuery(timestampNames[i]);
            RenderBackend::ExecuteCommandList(commandList);
        }

        {
            /* Lighting render pass */
            auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
            commandList->BeginTimestampQuery(timestampNames[2 + i]);

            // Transition all shadow maps to the pixel shader resource state
            for (std::size_t i = 0; i < s_Data.LightCount; ++i)
            {
                commandList->Transition(*s_Data.LightSubmissions[i].ShadowMap, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
            }

            // Set bindless descriptor heap
            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, bindlessDescriptorHeap);

            // Bind render pass bindables (sets viewport, scissor rect, render targets, pipeline state, root signature and primitive topology)
            commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::LIGHTING]);

            CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(g_RenderState.Settings.RenderResolution.x),
                static_cast<float>(g_RenderState.Settings.RenderResolution.y), 0.0f, 1.0f);
            CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

            commandList->SetViewports(1, &viewport);
            commandList->SetScissorRects(1, &scissorRect);

            D3D12_CPU_DESCRIPTOR_HANDLE rtv = g_RenderState.HDRColorTarget->GetDescriptor(DescriptorType::RTV);
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV);
            commandList->SetRenderTargets(1, &rtv, &dsv);

            // Set root CBV for constant buffers
            commandList->SetRootConstantBufferView(0, *g_RenderState.SceneDataConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(1, *g_RenderState.MaterialConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(2, *g_RenderState.LightConstantBuffer, D3D12_RESOURCE_STATE_COMMON);

            // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
            commandList->SetRootDescriptorTable(3, bindlessDescriptorHeap.GetGPUBaseDescriptor());

            RenderGeometry(*commandList, s_Data.SceneCamera, static_cast<TransparencyMode>(i));

            commandList->EndTimestampQuery(timestampNames[2 + i]);
            RenderBackend::ExecuteCommandList(commandList);
        }
    }

    {
        /* Tone mapping render pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Tonemapping");

        // Transition the HDR render target to pixel shader resource state
        commandList->Transition(*g_RenderState.HDRColorTarget, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
        commandList->Transition(*g_RenderState.SDRColorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->GetGraphicsCommandList()->SetComputeRootSignature(s_Data.ComputePasses[ComputePassType::TONE_MAPPING]->GetD3D12RootSignature().Get());
        commandList->GetGraphicsCommandList()->SetPipelineState(s_Data.ComputePasses[ComputePassType::TONE_MAPPING]->GetD3D12PipelineState().Get());

        ID3D12DescriptorHeap* const heaps = { RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetD3D12DescriptorHeap().Get() };
        commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &heaps);

        s_Data.TonemapSettings.HDRRenderTargetIndex = g_RenderState.HDRColorTarget->GetDescriptorHeapIndex(DescriptorType::SRV);
        s_Data.TonemapSettings.SDRRenderTargetIndex = g_RenderState.SDRColorTarget->GetDescriptorHeapIndex(DescriptorType::UAV);
        g_RenderState.TonemapConstantBuffer->SetBufferData(&s_Data.TonemapSettings);

        commandList->GetGraphicsCommandList()->SetComputeRootConstantBufferView(0, g_RenderState.TonemapConstantBuffer->GetD3D12Resource()->GetGPUVirtualAddress());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(1, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(2, RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetGPUBaseDescriptor());

        uint32_t threadX = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.x, 8) / 8;
        uint32_t threadY = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.y, 8) / 8;
        commandList->GetGraphicsCommandList()->Dispatch(threadX, threadY, 1);

        commandList->EndTimestampQuery("Tonemapping");
        RenderBackend::ExecuteCommandList(commandList);
    }
}

void Renderer::OnImGuiRender()
{
    auto& renderSettings = g_RenderState.Settings;
    auto& renderStats = g_RenderState.Stats;

    ImGui::Text("Renderer");
    ImGui::Text("Render resolution: %ux%u", renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
    ImGui::Text("VSync: %s", renderSettings.VSync ? "On" : "Off");
    ImGui::Text("Shadow map resolution: %ux%u", renderSettings.ShadowMapResolution, renderSettings.ShadowMapResolution);

    if (ImGui::CollapsingHeader("Stats"))
    {
        ImGui::Text("Draw calls: %u", renderStats.DrawCallCount);
        ImGui::Text("Triangle count: %u", renderStats.TriangleCount);
        ImGui::Text("Mesh count: %u", renderStats.MeshCount);
        ImGui::Text("Directional light count: %u", s_Data.SceneData.DirLightCount);
        ImGui::Text("Point light count: %u", s_Data.SceneData.PointLightCount);
        ImGui::Text("Spot light count: %u", s_Data.SceneData.SpotLightCount);
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

    s_Data.OpaqueMeshCount = 0;
    s_Data.TransparentMeshCount = 0;
    s_Data.MaterialCount = 0;
    s_Data.LightCount = 0;

    s_Data.SceneData.Reset();
}

void Renderer::EndFrame()
{
    SCOPED_TIMER("Renderer::EndFrame");

    RenderBackend::GetSwapChain().ResolveToBackBuffer(*g_RenderState.SDRColorTarget);
    RenderBackend::EndFrame();

    g_RenderState.Stats.Reset();
}

void Renderer::Submit(RenderResourceHandle meshPrimitiveHandle, const glm::mat4& transform)
{
    ASSERT(s_Data.MaterialCount <= g_RenderState.MAX_MATERIALS, "Total number of materials has exceeded the maximum amount of 1000");

    Mesh* mesh = g_RenderState.MeshSlotmap.Find(meshPrimitiveHandle);
    Material* material = g_RenderState.MaterialSlotmap.Find(mesh->Material);

    uint32_t albedoTextureIndex = g_RenderState.DefaultWhiteTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    Texture* albedoTexture = g_RenderState.TextureSlotmap.Find(material->AlbedoTexture);

    if (albedoTexture && albedoTexture->IsValid())
        albedoTextureIndex = albedoTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    uint32_t normalTextureIndex = g_RenderState.DefaultNormalTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    Texture* normalTexture = g_RenderState.TextureSlotmap.Find(material->NormalTexture);

    if (normalTexture && normalTexture->IsValid())
        normalTextureIndex = normalTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    uint32_t metallicRoughnessTextureIndex = g_RenderState.DefaultWhiteTexture->GetDescriptorHeapIndex(DescriptorType::SRV);
    Texture* metallicRoughnessTexture = g_RenderState.TextureSlotmap.Find(material->MetallicRoughnessTexture);

    if (metallicRoughnessTexture && metallicRoughnessTexture->IsValid())
        metallicRoughnessTextureIndex = metallicRoughnessTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    MaterialData materialData = {};
    materialData.AlbedoTextureIndex = albedoTextureIndex;
    materialData.NormalTextureIndex = normalTextureIndex;
    materialData.MetallicRoughnessTextureIndex = metallicRoughnessTextureIndex;
    materialData.Metalness = material->MetalnessFactor;
    materialData.Roughness = material->RoughnessFactor;

    g_RenderState.MaterialConstantBuffer->SetBufferDataAtOffset(&materialData, sizeof(MaterialData), s_Data.MaterialCount * sizeof(MaterialData));

    MeshInstanceData meshInstance = {};
    meshInstance.Transform = transform;
    meshInstance.MaterialID = s_Data.MaterialCount;

    if (material->Transparency == TransparencyMode::OPAQUE)
    {
        ASSERT(s_Data.OpaqueMeshCount <= g_RenderState.MAX_MESH_INSTANCES, "Exceeded the maximum amount of mesh instances for opaque meshes");

        g_RenderState.MeshInstanceBufferOpaque->SetBufferDataAtOffset(&meshInstance, sizeof(MeshInstanceData), s_Data.OpaqueMeshCount * sizeof(MeshInstanceData));
        s_Data.OpaqueMeshSubmissions[s_Data.OpaqueMeshCount].Mesh = mesh;
        s_Data.OpaqueMeshSubmissions[s_Data.OpaqueMeshCount].InstanceData.Transform = transform;
        s_Data.OpaqueMeshSubmissions[s_Data.OpaqueMeshCount].InstanceData.MaterialID = s_Data.MaterialCount;

        s_Data.OpaqueMeshCount++;
    }
    else
    {
        ASSERT(s_Data.TransparentMeshCount <= g_RenderState.MAX_MESH_INSTANCES, "Exceeded the maximum amount of mesh instances for transparent meshes");

        g_RenderState.MeshInstanceBufferTransparent->SetBufferDataAtOffset(&meshInstance, sizeof(MeshInstanceData), s_Data.TransparentMeshCount * sizeof(MeshInstanceData));
        s_Data.TransparentMeshSubmissions[s_Data.TransparentMeshCount].Mesh = mesh;
        s_Data.TransparentMeshSubmissions[s_Data.TransparentMeshCount].InstanceData.Transform = transform;
        s_Data.TransparentMeshSubmissions[s_Data.TransparentMeshCount].InstanceData.MaterialID = s_Data.MaterialCount;

        s_Data.TransparentMeshCount++;
    }

    s_Data.MaterialCount++;
}

void Renderer::Submit(DirectionalLightData& dirLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle)
{
    ASSERT(s_Data.SceneData.DirLightCount <= g_RenderState.MAX_DIR_LIGHTS, "Exceeded the maximum amount of directional lights");

    Texture* shadowMapTexture = g_RenderState.TextureSlotmap.Find(shadowMapHandle);
    if (shadowMapTexture)
        dirLightData.ShadowMapIndex = shadowMapTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    g_RenderState.LightConstantBuffer->SetBufferDataAtOffset(&dirLightData, sizeof(DirectionalLightData), 0);
    s_Data.LightSubmissions[s_Data.LightCount].ShadowMap = shadowMapTexture;
    s_Data.LightSubmissions[s_Data.LightCount].Face = 0;
    s_Data.LightSubmissions[s_Data.LightCount].LightCamera = lightCamera;

    s_Data.SceneData.DirLightCount++;
    s_Data.LightCount++;
}

void Renderer::Submit(SpotLightData& spotLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle)
{
    ASSERT(s_Data.SceneData.SpotLightCount <= g_RenderState.MAX_SPOT_LIGHTS, "Exceeded the maximum amount of spotlights");

    Texture* shadowMapTexture = g_RenderState.TextureSlotmap.Find(shadowMapHandle);
    if (shadowMapTexture)
        spotLightData.ShadowMapIndex = shadowMapTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    g_RenderState.LightConstantBuffer->SetBufferDataAtOffset(&spotLightData, sizeof(SpotLightData),
        sizeof(DirectionalLightData) + s_Data.SceneData.SpotLightCount * sizeof(SpotLightData));
    s_Data.LightSubmissions[s_Data.LightCount].ShadowMap = shadowMapTexture;
    s_Data.LightSubmissions[s_Data.LightCount].Face = 0;
    s_Data.LightSubmissions[s_Data.LightCount].LightCamera = lightCamera;

    s_Data.SceneData.SpotLightCount++;
    s_Data.LightCount++;
}

void Renderer::Submit(PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, RenderResourceHandle shadowMapHandle)
{
    ASSERT(s_Data.SceneData.PointLightCount <= g_RenderState.MAX_POINT_LIGHTS, "Exceeded the maximum amount of pointlights");

    Texture* shadowMapTexture = g_RenderState.TextureSlotmap.Find(shadowMapHandle);
    if (shadowMapTexture)
        pointLightData.ShadowMapIndex = shadowMapTexture->GetDescriptorHeapIndex(DescriptorType::SRV);

    g_RenderState.LightConstantBuffer->SetBufferDataAtOffset(&pointLightData, sizeof(PointLightData),
        sizeof(DirectionalLightData) + g_RenderState.MAX_SPOT_LIGHTS * sizeof(SpotLightData) + s_Data.SceneData.PointLightCount * sizeof(PointLightData));

    for (std::size_t i = 0; i < 6; ++i)
    {
        s_Data.LightSubmissions[s_Data.LightCount + i].ShadowMap = shadowMapTexture;
        s_Data.LightSubmissions[s_Data.LightCount + i].Face = i;
        s_Data.LightSubmissions[s_Data.LightCount + i].LightCamera = lightCameras[i];
    }

    s_Data.SceneData.PointLightCount++;
    s_Data.LightCount += 6;
}

RenderResourceHandle Renderer::CreateBuffer(const BufferDesc& desc)
{
    return g_RenderState.BufferSlotmap.Insert(desc);
}

RenderResourceHandle Renderer::CreateTexture(const TextureDesc& desc)
{
    return g_RenderState.TextureSlotmap.Insert(desc);
}

RenderResourceHandle Renderer::CreateMesh(const MeshDesc& desc)
{
    Mesh mesh = {};
    mesh.VertexBuffer = CreateBuffer(desc.VertexBufferDesc);
    mesh.IndexBuffer = CreateBuffer(desc.IndexBufferDesc);
    mesh.Material = desc.MaterialHandle;
    mesh.BB = desc.BB;
    mesh.DebugName = desc.DebugName;

    return g_RenderState.MeshSlotmap.Insert(mesh);
}

RenderResourceHandle Renderer::CreateMaterial(const MaterialDesc& desc)
{
    Material material = {};
    material.AlbedoTexture = CreateTexture(desc.AlbedoDesc);
    material.NormalTexture = CreateTexture(desc.NormalDesc);
    material.MetallicRoughnessTexture = CreateTexture(desc.MetallicRoughnessDesc);
    material.MetalnessFactor = desc.Metalness;
    material.RoughnessFactor = desc.Roughness;
    material.Transparency = desc.Transparency;

    return g_RenderState.MaterialSlotmap.Insert(material);
}

void Renderer::Resize(uint32_t width, uint32_t height)
{
    if (g_RenderState.Settings.RenderResolution.x != width || g_RenderState.Settings.RenderResolution.y != height)
    {
        g_RenderState.Settings.RenderResolution.x = std::max(1u, width);
        g_RenderState.Settings.RenderResolution.y = std::max(1u, height);

        RenderBackend::Resize(g_RenderState.Settings.RenderResolution.x, g_RenderState.Settings.RenderResolution.y);
        ResizeResolutionDependentResources(g_RenderState.Settings.RenderResolution.x, g_RenderState.Settings.RenderResolution.y);
    }
}

void Renderer::ToggleVSync()
{
    g_RenderState.Settings.VSync = !g_RenderState.Settings.VSync;
    RenderBackend::SetVSync(g_RenderState.Settings.VSync);
}

bool Renderer::IsVSyncEnabled()
{
    return g_RenderState.Settings.VSync;
}

const Resolution& Renderer::GetRenderResolution()
{
    return g_RenderState.Settings.RenderResolution;
}

const Resolution& Renderer::GetShadowResolution()
{
    return g_RenderState.Settings.ShadowMapResolution;
}
