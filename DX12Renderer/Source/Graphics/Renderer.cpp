#include "Pch.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderState.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/ResourceSlotmap.h"
#include "Graphics/DebugRenderer.h"
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

static constexpr glm::vec2 HaltonJitterSamples[16] = {
    { 0.500000f, 0.333333f },
    { 0.250000f, 0.666667f },
    { 0.750000f, 0.111111f },
    { 0.125000f, 0.444444f },
    { 0.625000f, 0.777778f },
    { 0.375000f, 0.222222f },
    { 0.875000f, 0.555556f },
    { 0.062500f, 0.888889f },
    { 0.562500f, 0.037037f },
    { 0.312500f, 0.370370f },
    { 0.812500f, 0.703704f },
    { 0.187500f, 0.148148f },
    { 0.687500f, 0.481481f },
    { 0.437500f, 0.814815f },
    { 0.937500f, 0.259259f },
    { 0.031250f, 0.592593f },
};

glm::vec2 GetRandomHaltonJitter(uint32_t renderWidth, uint32_t renderHeight)
{
    return (2.0f * HaltonJitterSamples[Random::UintRange(0, 15)] - 1.0f) / glm::vec2(renderWidth, renderHeight);
}

enum RenderPassType : uint32_t
{
    SHADOW_MAPPING,
    DEPTH_PREPASS,
    LIGHTING,
    NUM_RENDER_PASSES
};

enum ComputePassType : uint32_t
{
    TEMPORAL_ANTI_ALIASING,
    POST_PROCESS,
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

struct InternalRendererData
{
    // Render passes
    std::unique_ptr<RasterPass> RenderPasses[RenderPassType::NUM_RENDER_PASSES];
    std::unique_ptr<ComputePass> ComputePasses[ComputePassType::NUM_COMPUTE_PASSES];

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
        D3D12_RENDER_TARGET_BLEND_DESC defaultBlendDesc = {};
        defaultBlendDesc.BlendEnable = TRUE;
        defaultBlendDesc.LogicOpEnable = FALSE;
        defaultBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
        defaultBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
        defaultBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
        defaultBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
        defaultBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
        defaultBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
        defaultBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
        defaultBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

        {
            // Shadow mapping render pass
            RasterPassDesc desc;
            desc.VertexShaderPath = "Resources/Shaders/ShadowMapping_VS.hlsl";
            desc.PixelShaderPath = "Resources/Shaders/ShadowMapping_PS.hlsl";

            desc.NumColorAttachments = 0;
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

            desc.NumColorAttachments = 0;
            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;

            desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_LESS;

            desc.RootParameters.resize(2);
            desc.RootParameters[0].InitAsConstantBufferView(0); // Global constant buffer
            desc.RootParameters[1].InitAsConstantBufferView(1); // Scene data constant buffer

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

            desc.NumColorAttachments = 2;
            desc.ColorBlendDesc[0] = defaultBlendDesc;
            desc.ColorAttachmentDesc[0].Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.ColorAttachmentDesc[0].Format = TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT;
            desc.ColorAttachmentDesc[0].Width = g_RenderState.Settings.RenderResolution.x;
            desc.ColorAttachmentDesc[0].Height = g_RenderState.Settings.RenderResolution.y;

            desc.ColorBlendDesc[1] = defaultBlendDesc;
            desc.ColorBlendDesc[1].BlendEnable = FALSE;
            desc.ColorAttachmentDesc[1].Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.ColorAttachmentDesc[1].Format = TextureFormat::TEXTURE_FORMAT_RG16_FLOAT;
            desc.ColorAttachmentDesc[1].Width = g_RenderState.Settings.RenderResolution.x;
            desc.ColorAttachmentDesc[1].Height = g_RenderState.Settings.RenderResolution.y;

            desc.DepthAttachmentDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH;
            desc.DepthAttachmentDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.DepthAttachmentDesc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.DepthAttachmentDesc.Height = g_RenderState.Settings.RenderResolution.y;

            desc.DepthComparisonFunc = D3D12_COMPARISON_FUNC_EQUAL;

            // Need to mark the bindless descriptor range as DESCRIPTORS_VOLATILE since it will contain empty descriptors
            CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

            desc.RootParameters.resize(5);
            desc.RootParameters[0].InitAsConstantBufferView(0); // Global constant buffer
            desc.RootParameters[1].InitAsConstantBufferView(1); // Scene data constant buffer
            desc.RootParameters[2].InitAsConstantBufferView(2); // Material constant buffer
            desc.RootParameters[3].InitAsConstantBufferView(3); // Light constant buffer
            desc.RootParameters[4].InitAsDescriptorTable(_countof(ranges), &ranges[0], D3D12_SHADER_VISIBILITY_PIXEL); // Bindless SRV table

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
            // Temporal anti-aliasing pass
            ComputePassDesc desc;
            desc.ComputeShaderPath = "Resources/Shaders/TemporalAA_CS.hlsl";
            desc.NumThreadsX = 64;
            desc.NumThreadsY = 64;
            desc.NumThreadsZ = 1;

            CD3DX12_DESCRIPTOR_RANGE1 ranges[6] = {};
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 1, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[2].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 2, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[3].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 3, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[4].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 4, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[5].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

            desc.RootParameters.resize(7);
            desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // Global constant buffer
            desc.RootParameters[1].InitAsDescriptorTable(1, &ranges[0]); // HDR color target (current frame)
            desc.RootParameters[2].InitAsDescriptorTable(1, &ranges[1]); // Depth target (current frame)
            desc.RootParameters[3].InitAsDescriptorTable(1, &ranges[2]); // Velocity target (current frame)
            desc.RootParameters[4].InitAsDescriptorTable(1, &ranges[3]); // Velocity target previous (last frame)
            desc.RootParameters[5].InitAsDescriptorTable(1, &ranges[4]); // TAA history (accumulated last frames)
            desc.RootParameters[6].InitAsDescriptorTable(1, &ranges[5]); // TAA resolve target (dest)

            s_Data.ComputePasses[ComputePassType::TEMPORAL_ANTI_ALIASING] = std::make_unique<ComputePass>("Temporal anti-aliasing", desc);
        }

        {
            // Tonemapping render pass
            ComputePassDesc desc;
            desc.ComputeShaderPath = "Resources/Shaders/PostProcess_CS.hlsl";
            desc.NumThreadsX = 64;
            desc.NumThreadsY = 64;
            desc.NumThreadsZ = 1;

            CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
            ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
            ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

            desc.RootParameters.resize(3);
            desc.RootParameters[0].InitAsConstantBufferView(0, 0, D3D12_ROOT_DESCRIPTOR_FLAG_DATA_STATIC_WHILE_SET_AT_EXECUTE); // Global constant buffer
            desc.RootParameters[1].InitAsDescriptorTable(1, &ranges[0]); // Source texture
            desc.RootParameters[2].InitAsDescriptorTable(1, &ranges[1]); // SDR color target (dest)

            s_Data.ComputePasses[ComputePassType::POST_PROCESS] = std::make_unique<ComputePass>("Post-process", desc);
        }
    }

    void CreateBuffers()
    {
        // TODO: Constant buffers should be replaced by a giant upload/constant buffer which can be suballocated from

        {
            // Global constant buffer
            BufferDesc desc = {};
            desc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
            desc.NumElements = 1 * g_RenderState.BACK_BUFFER_COUNT;
            desc.ElementSize = sizeof(GlobalConstantBufferData);
            desc.DebugName = "Global constant buffer";

            g_RenderState.GlobalConstantBuffer = std::make_unique<Buffer>(desc);
        }
        
        {
            // Mesh instance buffers (opaque and transparent meshes)
            BufferDesc desc = {};
            desc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
            //meshBufferDesc.NumElements = g_RenderState.MAX_MESH_INSTANCES * g_RenderState.BACK_BUFFER_COUNT;
            desc.NumElements = g_RenderState.MAX_MESH_INSTANCES;
            desc.ElementSize = sizeof(MeshInstanceData);
            desc.DebugName = "Mesh instance buffer opaque";
            g_RenderState.MeshInstanceBufferOpaque = std::make_unique<Buffer>(desc);

            desc.DebugName = "Mesh instance buffer transparent";
            g_RenderState.MeshInstanceBufferTransparent = std::make_unique<Buffer>(desc);
        }

        {
            // Material constant buffer
            BufferDesc desc = {};
            desc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
            //materialBufferDesc.NumElements = g_RenderState.MAX_MATERIALS * g_RenderState.BACK_BUFFER_COUNT;
            desc.NumElements = g_RenderState.MAX_MATERIALS;
            desc.ElementSize = sizeof(MaterialData);
            desc.DebugName = "Material constant buffer";

            g_RenderState.MaterialConstantBuffer = std::make_unique<Buffer>(desc);
        }

        {
            // Light constant buffer
            BufferDesc desc = {};
            desc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
            desc.NumElements = g_RenderState.MAX_DIR_LIGHTS * sizeof(DirectionalLightData) +
                g_RenderState.MAX_SPOT_LIGHTS * sizeof(SpotLightData) + g_RenderState.MAX_POINT_LIGHTS * sizeof(PointLightData);
            desc.ElementSize = 1 * g_RenderState.BACK_BUFFER_COUNT;
            desc.DebugName = "Light constant buffer";

            g_RenderState.LightConstantBuffer = std::make_unique<Buffer>(desc);
        }

        {
            // Scene data constant buffer
            BufferDesc desc = {};
            desc.Usage = BufferUsage::BUFFER_USAGE_CONSTANT;
            desc.NumElements = 1 * g_RenderState.BACK_BUFFER_COUNT;
            desc.ElementSize = sizeof(SceneData);
            desc.DebugName = "Scene data constant buffer";

            g_RenderState.SceneDataConstantBuffer = std::make_unique<Buffer>(desc);
        }
    }

    void CreateRenderTargets()
    {
        {
            // Depth pre-pass depth target
            TextureDesc desc = {};
            desc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ;
            desc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
            desc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.ClearColor = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
            desc.DebugName = "Depth pre-pass depth target";

            g_RenderState.DepthPrepassDepthTarget = std::make_unique<Texture>(desc);
        }

        {
            // HDR color target, TAA resolve target, TAA history
            TextureDesc desc = {};
            desc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.Format = TextureFormat::TEXTURE_FORMAT_RGBA16_FLOAT;
            desc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.DebugName = "HDR color target";

            g_RenderState.HDRColorTarget = std::make_unique<Texture>(desc);

            desc.Usage = TextureUsage::TEXTURE_USAGE_READ | TextureUsage::TEXTURE_USAGE_WRITE;
            desc.DebugName = "TAA resolve target";
            g_RenderState.TAAResolveTarget = std::make_unique<Texture>(desc);

            desc.Usage = TextureUsage::TEXTURE_USAGE_READ;
            desc.DebugName = "TAA history";
            g_RenderState.TAAHistory = std::make_unique<Texture>(desc);
        }

        {
            // SDR color target
            TextureDesc desc = {};
            desc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_WRITE;
            desc.Format = TextureFormat::TEXTURE_FORMAT_RGBA8_UNORM;
            desc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.DebugName = "SDR color target";

            g_RenderState.SDRColorTarget = std::make_unique<Texture>(desc);
        }

        {
            // Velocity target
            TextureDesc desc = {};
            desc.Usage = TextureUsage::TEXTURE_USAGE_RENDER_TARGET | TextureUsage::TEXTURE_USAGE_READ;
            desc.Format = TextureFormat::TEXTURE_FORMAT_RG16_FLOAT;
            desc.Width = g_RenderState.Settings.RenderResolution.x;
            desc.Height = g_RenderState.Settings.RenderResolution.y;
            desc.DebugName = "Velocity target";

            g_RenderState.VelocityTarget = std::make_unique<Texture>(desc);

            desc.Usage = TextureUsage::TEXTURE_USAGE_READ;
            desc.DebugName = "Velocity target previous";

            g_RenderState.VelocityTargetPrevious = std::make_unique<Texture>(desc);
        }
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
        uint32_t defaultNormalTextureData = (255 << 24) | (255 << 16) | (127 << 8) | (127 << 0);
        defaultTextureDesc.DataPtr = &defaultNormalTextureData;
        defaultTextureDesc.DebugName = "Default normal texture";

        g_RenderState.DefaultNormalTexture = std::make_unique<Texture>(defaultTextureDesc);
    }

    void CopyPreviousFrameRenderTargets()
    {
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        // Copy current velocity into previous velocity
        commandList->CopyResource(*g_RenderState.VelocityTargetPrevious, *g_RenderState.VelocityTarget);

        RenderBackend::ExecuteCommandList(commandList);
    }

    void ClearRenderTargets()
    {
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);
        commandList->Transition(*g_RenderState.VelocityTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->Transition(*g_RenderState.HDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->Transition(*g_RenderState.SDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);

        commandList->ClearDepthStencilView(g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV), g_RenderState.DepthPrepassDepthTarget->GetTextureDesc().ClearColor.x);
        commandList->ClearRenderTargetView(g_RenderState.VelocityTarget->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(g_RenderState.VelocityTarget->GetTextureDesc().ClearColor));
        commandList->ClearRenderTargetView(g_RenderState.HDRColorTarget->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(g_RenderState.HDRColorTarget->GetTextureDesc().ClearColor));
        commandList->ClearRenderTargetView(g_RenderState.SDRColorTarget->GetDescriptor(DescriptorType::RTV), glm::value_ptr<float>(g_RenderState.SDRColorTarget->GetTextureDesc().ClearColor));

        RenderBackend::ExecuteCommandList(commandList);
    }

    void ResizeResolutionDependentResources(uint32_t width, uint32_t height)
    {
        g_RenderState.DepthPrepassDepthTarget->Resize(width, height);
        g_RenderState.HDRColorTarget->Resize(width, height);
        g_RenderState.SDRColorTarget->Resize(width, height);

        g_RenderState.TAAResolveTarget->Resize(width, height);
        g_RenderState.TAAHistory->Resize(width, height);
        g_RenderState.VelocityTarget->Resize(width, height);
        g_RenderState.VelocityTargetPrevious->Resize(width, height);
    }

    bool CullViewFrustum(const ViewFrustum& frustum, const Mesh* mesh, const MeshInstanceData& meshInstance)
    {
        BoundingBox meshInstanceBB = mesh->BB;
        meshInstanceBB.Min = meshInstance.Transform * glm::vec4(mesh->BB.Min, 1.0f);
        meshInstanceBB.Max = meshInstance.Transform * glm::vec4(mesh->BB.Max, 1.0f);

        DebugRenderer::SubmitAABB(meshInstanceBB.Min, meshInstanceBB.Max, glm::vec4(1.0f, 0.0f, 1.0f, 1.0f));

        return frustum.IsBoxInViewFrustum(meshInstanceBB.Min, meshInstanceBB.Max);
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
            const auto mesh = (*meshSubmissions)[m].Mesh;
            const auto& meshInstance = (*meshSubmissions)[m].InstanceData;

            if (camera.IsFrustumCullingEnabled())
            {
                if (!CullViewFrustum(camera.GetViewFrustum(), mesh, meshInstance))
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

    CopyPreviousFrameRenderTargets();
    ClearRenderTargets();
}

void Renderer::BeginScene(const Camera& sceneCamera)
{
    g_RenderState.GlobalCBData.PrevViewProj = g_RenderState.GlobalCBData.ViewProj;
    g_RenderState.GlobalCBData.PrevInvViewProj = g_RenderState.GlobalCBData.InvViewProj;

    g_RenderState.GlobalCBData.ViewProj = sceneCamera.GetViewProjection();
    g_RenderState.GlobalCBData.InvViewProj = glm::inverse(g_RenderState.GlobalCBData.ViewProj);

    s_Data.SceneCamera = sceneCamera;
    s_Data.SceneData.ViewProjection = sceneCamera.GetViewProjection();
    s_Data.SceneData.CameraPosition = sceneCamera.GetTransform().GetPosition();
}

void Renderer::Render()
{
    SCOPED_TIMER("Renderer::Render");

    g_RenderState.GlobalCBData.Resolution = g_RenderState.Settings.RenderResolution;
    g_RenderState.GlobalCBData.TAA_HaltonJitter = GetRandomHaltonJitter(g_RenderState.Settings.RenderResolution.x, g_RenderState.Settings.RenderResolution.y);
    g_RenderState.GlobalCBData.TM_Exposure = s_Data.SceneCamera.GetExposure();
    g_RenderState.GlobalCBData.TM_Gamma = s_Data.SceneCamera.GetGamma();
    g_RenderState.GlobalConstantBuffer->SetBufferData(&g_RenderState.GlobalCBData);

    g_RenderState.SceneDataConstantBuffer->SetBufferData(&s_Data.SceneData);
    g_RenderState.Stats.MeshCount = s_Data.OpaqueMeshCount + s_Data.TransparentMeshCount;

    for (std::size_t i = 0; i < s_Data.OpaqueMeshCount; ++i)
    {
        Buffer* indexBuffer = g_RenderState.BufferSlotmap.Find(s_Data.OpaqueMeshSubmissions[i].Mesh->IndexBuffer);
        g_RenderState.Stats.TriangleCount += indexBuffer->GetBufferDesc().NumElements / 3;
    }

    for (std::size_t i = 0; i < s_Data.TransparentMeshCount; ++i)
    {
        Buffer* indexBuffer = g_RenderState.BufferSlotmap.Find(s_Data.TransparentMeshSubmissions[i].Mesh->IndexBuffer);
        g_RenderState.Stats.TriangleCount += indexBuffer->GetBufferDesc().NumElements / 3;
    }

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

            commandList->SetRootConstantBufferView(0, *g_RenderState.GlobalConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(1, *g_RenderState.SceneDataConstantBuffer, D3D12_RESOURCE_STATE_COMMON);

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

            commandList->Transition(*g_RenderState.HDRColorTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->Transition(*g_RenderState.VelocityTarget, D3D12_RESOURCE_STATE_RENDER_TARGET);
            commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_DEPTH_WRITE);

            // Set bindless descriptor heap
            commandList->SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, bindlessDescriptorHeap);

            // Bind render pass bindables (sets viewport, scissor rect, render targets, pipeline state, root signature and primitive topology)
            commandList->SetRenderPassBindables(*s_Data.RenderPasses[RenderPassType::LIGHTING]);

            CD3DX12_VIEWPORT viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(g_RenderState.Settings.RenderResolution.x),
                static_cast<float>(g_RenderState.Settings.RenderResolution.y), 0.0f, 1.0f);
            CD3DX12_RECT scissorRect = CD3DX12_RECT(0.0f, 0.0f, LONG_MAX, LONG_MAX);

            commandList->SetViewports(1, &viewport);
            commandList->SetScissorRects(1, &scissorRect);

            D3D12_CPU_DESCRIPTOR_HANDLE rtvs[] = {
                g_RenderState.HDRColorTarget->GetDescriptor(DescriptorType::RTV),
                g_RenderState.VelocityTarget->GetDescriptor(DescriptorType::RTV)
            };
            D3D12_CPU_DESCRIPTOR_HANDLE dsv = g_RenderState.DepthPrepassDepthTarget->GetDescriptor(DescriptorType::DSV);
            commandList->SetRenderTargets(2, rtvs, &dsv);

            // Set root CBV for constant buffers
            commandList->SetRootConstantBufferView(0, *g_RenderState.GlobalConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(1, *g_RenderState.SceneDataConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(2, *g_RenderState.MaterialConstantBuffer, D3D12_RESOURCE_STATE_COMMON);
            commandList->SetRootConstantBufferView(3, *g_RenderState.LightConstantBuffer, D3D12_RESOURCE_STATE_COMMON);

            // Set root descriptor table for bindless CBV_SRV_UAV descriptor array
            commandList->SetRootDescriptorTable(4, bindlessDescriptorHeap.GetGPUBaseDescriptor());

            RenderGeometry(*commandList, s_Data.SceneCamera, static_cast<TransparencyMode>(i));

            commandList->EndTimestampQuery(timestampNames[2 + i]);
            RenderBackend::ExecuteCommandList(commandList);
        }
    }

    {
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);

        if (g_RenderState.Settings.EnableTAA)
        {
            /* Temporal anti-aliasing pass */
            commandList->BeginTimestampQuery("Temporal-AA");

            // Transition sdr history and sdr target into correct states
            commandList->Transition(*g_RenderState.HDRColorTarget, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            commandList->Transition(*g_RenderState.DepthPrepassDepthTarget, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            commandList->Transition(*g_RenderState.VelocityTarget, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            commandList->Transition(*g_RenderState.VelocityTargetPrevious, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            commandList->Transition(*g_RenderState.TAAHistory, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
            commandList->Transition(*g_RenderState.TAAResolveTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

            commandList->GetGraphicsCommandList()->SetComputeRootSignature(s_Data.ComputePasses[ComputePassType::TEMPORAL_ANTI_ALIASING]->GetD3D12RootSignature().Get());
            commandList->GetGraphicsCommandList()->SetPipelineState(s_Data.ComputePasses[ComputePassType::TEMPORAL_ANTI_ALIASING]->GetD3D12PipelineState().Get());

            ID3D12DescriptorHeap* const heaps = { RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetD3D12DescriptorHeap().Get() };
            commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &heaps);

            commandList->GetGraphicsCommandList()->SetComputeRootConstantBufferView(0, g_RenderState.GlobalConstantBuffer->GetD3D12Resource()->GetGPUVirtualAddress());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(1, g_RenderState.HDRColorTarget->GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(2, g_RenderState.DepthPrepassDepthTarget->GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(3, g_RenderState.VelocityTarget->GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(4, g_RenderState.VelocityTargetPrevious->GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(5, g_RenderState.TAAHistory->GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
            commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(6, g_RenderState.TAAResolveTarget->GetDescriptorAllocation(DescriptorType::UAV).GetGPUDescriptorHandle());

            uint32_t threadX = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.x, 8) / 8;
            uint32_t threadY = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.y, 8) / 8;
            commandList->GetGraphicsCommandList()->Dispatch(threadX, threadY, 1);

            commandList->EndTimestampQuery("Temporal-AA");
        }
        else
        {
            commandList->CopyResource(*g_RenderState.TAAResolveTarget, *g_RenderState.HDRColorTarget);
        }

        // Update TAA history with the current TAA resolve
        commandList->CopyResource(*g_RenderState.TAAHistory, *g_RenderState.TAAResolveTarget);
        RenderBackend::ExecuteCommandList(commandList);
    }

    {
        /* Post-process pass */
        auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
        commandList->BeginTimestampQuery("Post-process");

        // Transition the HDR render target to pixel shader resource state
        commandList->Transition(*g_RenderState.TAAResolveTarget, D3D12_RESOURCE_STATE_ALL_SHADER_RESOURCE);
        commandList->Transition(*g_RenderState.SDRColorTarget, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);

        commandList->GetGraphicsCommandList()->SetComputeRootSignature(s_Data.ComputePasses[ComputePassType::POST_PROCESS]->GetD3D12RootSignature().Get());
        commandList->GetGraphicsCommandList()->SetPipelineState(s_Data.ComputePasses[ComputePassType::POST_PROCESS]->GetD3D12PipelineState().Get());

        ID3D12DescriptorHeap* const heaps = { RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV).GetD3D12DescriptorHeap().Get() };
        commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &heaps);

        commandList->GetGraphicsCommandList()->SetComputeRootConstantBufferView(0, g_RenderState.GlobalConstantBuffer->GetD3D12Resource()->GetGPUVirtualAddress());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(1, GetDebugShowDebugModeTexture(g_RenderState.GlobalCBData.PP_DebugShowTextureMode).GetDescriptorAllocation(DescriptorType::SRV).GetGPUDescriptorHandle());
        commandList->GetGraphicsCommandList()->SetComputeRootDescriptorTable(2, g_RenderState.SDRColorTarget->GetDescriptorAllocation(DescriptorType::UAV).GetGPUDescriptorHandle());

        uint32_t threadX = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.x, 8) / 8;
        uint32_t threadY = MathHelper::AlignUp(g_RenderState.Settings.RenderResolution.y, 8) / 8;
        commandList->GetGraphicsCommandList()->Dispatch(threadX, threadY, 1);

        commandList->EndTimestampQuery("Post-process");
        RenderBackend::ExecuteCommandList(commandList);
    }
}

void Renderer::OnImGuiRender()
{
    auto& renderSettings = g_RenderState.Settings;
    auto& renderStats = g_RenderState.Stats;

    ImGui::SetNextItemOpen(true, ImGuiCond_Once);
    if (ImGui::CollapsingHeader("Settings"))
    {
        ImGui::Indent(10.0f);

        ImGui::Text("Render resolution: %ux%u", renderSettings.RenderResolution.x, renderSettings.RenderResolution.y);
        ImGui::Text("Shadow map resolution: %ux%u", renderSettings.ShadowMapResolution, renderSettings.ShadowMapResolution);
        ImGui::Text("VSync: %s", renderSettings.EnableVSync ? "On" : "Off");

        ImGui::Text("Debug show texture");
        std::string previewMode = DebugShowTextureModeToString(g_RenderState.GlobalCBData.PP_DebugShowTextureMode);
        if (ImGui::BeginCombo("##DebugShowTexture", previewMode.c_str()))
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(DebugShowTextureMode::DEBUG_SHOW_TEXTURE_MODE_NUM_MODES); ++i)
            {
                std::string mode = DebugShowTextureModeToString(static_cast<DebugShowTextureMode>(i));
                bool isSelected = previewMode == mode;

                if (ImGui::Selectable(mode.c_str(), isSelected))
                    g_RenderState.GlobalCBData.PP_DebugShowTextureMode = static_cast<DebugShowTextureMode>(i);
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::Separator();

        ImGui::Checkbox("Temporal AA", &renderSettings.EnableTAA);
        ImGui::DragFloat("TAA source weight", &g_RenderState.GlobalCBData.TAA_SourceWeight, 0.01f, 0.0f, 1.0f);

        bool neighborhoodClamping = g_RenderState.GlobalCBData.TAA_UseNeighborhoodClamp;
        if (ImGui::Checkbox("Use neighborhood clamping", &neighborhoodClamping))
            g_RenderState.GlobalCBData.TAA_UseNeighborhoodClamp = neighborhoodClamping;

        bool useVelocityRejection = g_RenderState.GlobalCBData.TAA_UseVelocityRejection;
        if (ImGui::Checkbox("Use velocity rejection", &useVelocityRejection))
            g_RenderState.GlobalCBData.TAA_UseVelocityRejection = useVelocityRejection;

        bool showVelocityDisocclusion = g_RenderState.GlobalCBData.TAA_ShowVelocityDisocclusion;
        if (ImGui::Checkbox("Show velocity disocclusion", &showVelocityDisocclusion))
            g_RenderState.GlobalCBData.TAA_ShowVelocityDisocclusion = showVelocityDisocclusion;
        
        if (useVelocityRejection)
            ImGui::DragFloat("Velocity rejection strength", &g_RenderState.GlobalCBData.TAA_VelocityRejectionStrength, 0.1f, 0.0f, 1000.0f);

        ImGui::Separator();

        ImGui::Text("Tonemapping type");
        std::string previewValue = TonemapTypeToString(g_RenderState.GlobalCBData.TM_Type);
        if (ImGui::BeginCombo("##TonemapType", previewValue.c_str()))
        {
            for (uint32_t i = 0; i < static_cast<uint32_t>(TonemapType::NUM_TYPES); ++i)
            {
                std::string tonemapName = TonemapTypeToString(static_cast<TonemapType>(i));
                bool isSelected = previewValue == tonemapName;

                if (ImGui::Selectable(tonemapName.c_str(), isSelected))
                    g_RenderState.GlobalCBData.TM_Type = static_cast<TonemapType>(i);
                if (isSelected)
                    ImGui::SetItemDefaultFocus();
            }

            ImGui::EndCombo();
        }

        ImGui::Unindent(10.0f);
    }

    if (ImGui::CollapsingHeader("Stats"))
    {
        ImGui::Indent(10.0f);

        ImGui::Text("Draw calls: %u", renderStats.DrawCallCount);
        ImGui::Text("Triangle count: %u", renderStats.TriangleCount);
        ImGui::Text("Mesh count: %u", renderStats.MeshCount);
        ImGui::Text("Directional light count: %u", s_Data.SceneData.DirLightCount);
        ImGui::Text("Point light count: %u", s_Data.SceneData.PointLightCount);
        ImGui::Text("Spot light count: %u", s_Data.SceneData.SpotLightCount);

        ImGui::Unindent(10.0f);
    }
}

void Renderer::EndScene()
{
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
        s_Data.LightSubmissions[s_Data.LightCount + i].Face = i + 1;
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
    g_RenderState.Settings.EnableVSync = !g_RenderState.Settings.EnableVSync;
    RenderBackend::SetVSync(g_RenderState.Settings.EnableVSync);
}

bool Renderer::IsVSyncEnabled()
{
    return g_RenderState.Settings.EnableVSync;
}

const Resolution& Renderer::GetRenderResolution()
{
    return g_RenderState.Settings.RenderResolution;
}

const Resolution& Renderer::GetShadowResolution()
{
    return g_RenderState.Settings.ShadowMapResolution;
}
