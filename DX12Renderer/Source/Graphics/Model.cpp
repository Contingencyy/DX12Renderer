#include "Pch.h"
#include "Graphics/Model.h"
#include "Graphics/Shader.h"
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "ResourceLoader.h"
#include "Application.h"
#include "Graphics/Renderer.h"

Model::Model(const std::string& filepath)
{
	m_TinyglTFModel = ResourceLoader::LoadModel(filepath);

	std::map<std::string, int>& meshAttribs = m_TinyglTFModel.meshes[0].primitives[0].attributes;
	std::vector<tinygltf::Accessor> accessors = m_TinyglTFModel.accessors;
	std::vector<tinygltf::BufferView> bufferViews = m_TinyglTFModel.bufferViews;
	std::vector<tinygltf::Buffer> buffers = m_TinyglTFModel.buffers;

	for (auto& attrib : meshAttribs)
	{
		uint32_t componentSize;
		uint32_t numComponents;

		switch (accessors[attrib.second].componentType)
		{
		case TINYGLTF_COMPONENT_TYPE_BYTE:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE:
			componentSize = 1;
			break;
		case TINYGLTF_COMPONENT_TYPE_SHORT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT:
			componentSize = 2;
			break;
		case TINYGLTF_COMPONENT_TYPE_INT:
		case TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT:
		case TINYGLTF_COMPONENT_TYPE_FLOAT:
			componentSize = 4;
			break;
		case TINYGLTF_COMPONENT_TYPE_DOUBLE:
			componentSize = 8;
			break;
		}

		switch (accessors[attrib.second].type)
		{
		case TINYGLTF_TYPE_SCALAR:
			numComponents = 1;
			break;
		case TINYGLTF_TYPE_VEC2:
			numComponents = 2;
			break;
		case TINYGLTF_TYPE_VEC3:
			numComponents = 3;
			break;
		case TINYGLTF_TYPE_VEC4:
		case TINYGLTF_TYPE_MAT2:
			numComponents = 4;
			break;
		case TINYGLTF_TYPE_MAT3:
			numComponents = 9;
			break;
		case TINYGLTF_TYPE_MAT4:
			numComponents = 16;
			break;
		}
	}

	CreateRootSignature();
	CreatePipelineState();

	CreateBuffers();
	CreateTextures();
}

Model::~Model()
{
}

void Model::Render()
{
	Application::Get().GetRenderer()->DrawModel(this);
}

void Model::CreateRootSignature()
{
	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

	CD3DX12_DESCRIPTOR_RANGE1 descRanges[1] = {};
	descRanges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameters[2] = {};
	rootParameters[0].InitAsConstants(16, 0, 0, D3D12_SHADER_VISIBILITY_VERTEX);
	rootParameters[1].InitAsDescriptorTable(1, descRanges, D3D12_SHADER_VISIBILITY_PIXEL);

	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	staticSamplers[0].MipLODBias = 0;
	staticSamplers[0].MaxAnisotropy = 0;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
	rootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], _countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;
	DX_CALL(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &serializedRootSig, &errorBlob));

	DX_CALL(Application::Get().GetRenderer()->GetD3D12Device()->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&m_d3d12RootSignature)));
}

void Model::CreatePipelineState()
{
	UINT compileFlags = 0;
#ifdef _DEBUG
	compileFlags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif

	Shader vertexShader(L"Resources/Shaders/Default_VS.hlsl", "main", "vs_5_1");
	Shader pixelShader(L"Resources/Shaders/Default_PS.hlsl", "main", "ps_5_1");

	D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 1, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "MODEL", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 0, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 16, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 2, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 32, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "MODEL", 3, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 48, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 },
		{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 2, 64, D3D12_INPUT_CLASSIFICATION_PER_INSTANCE_DATA, 1 }
	};

	D3D12_RENDER_TARGET_BLEND_DESC rtBlendDesc = {};
	rtBlendDesc.BlendEnable = TRUE;
	rtBlendDesc.LogicOpEnable = FALSE;
	rtBlendDesc.SrcBlend = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOp = D3D12_BLEND_OP_ADD;
	rtBlendDesc.SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	rtBlendDesc.DestBlendAlpha = D3D12_BLEND_INV_SRC_ALPHA;
	rtBlendDesc.BlendOpAlpha = D3D12_BLEND_OP_ADD;
	rtBlendDesc.LogicOp = D3D12_LOGIC_OP_NOOP;
	rtBlendDesc.RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

	D3D12_BLEND_DESC blendDesc = {};
	blendDesc.AlphaToCoverageEnable = FALSE;
	blendDesc.IndependentBlendEnable = FALSE;
	blendDesc.RenderTarget[0] = rtBlendDesc;

	D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
	psoDesc.VS = vertexShader.GetShaderByteCode();
	psoDesc.PS = pixelShader.GetShaderByteCode();
	psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	psoDesc.BlendState = blendDesc;
	psoDesc.DepthStencilState.DepthEnable = TRUE;
	psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
	psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
	psoDesc.DepthStencilState.StencilEnable = FALSE;
	psoDesc.SampleMask = UINT_MAX;
	psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	psoDesc.NumRenderTargets = 1;
	psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
	psoDesc.SampleDesc.Count = 1;
	psoDesc.pRootSignature = m_d3d12RootSignature.Get();

	DX_CALL(Application::Get().GetRenderer()->GetD3D12Device()->CreateGraphicsPipelineState(&psoDesc, IID_PPV_ARGS(&m_d3d12PipelineState)));
}

void Model::CreateBuffers()
{
	// Load vertex attributes from glTF model
	std::string attributeNames[] = { "POSITION", "TEXCOORD_0" };

	for (const std::string& attributeName : attributeNames)
	{
		auto iter = m_TinyglTFModel.meshes[0].primitives[0].attributes.find(attributeName);
		if (iter != m_TinyglTFModel.meshes[0].primitives[0].attributes.end())
		{
			uint32_t attributeIndex = iter->second;
			auto& accessor = m_TinyglTFModel.accessors[attributeIndex];
			auto& bufferView = m_TinyglTFModel.bufferViews[accessor.bufferView];
			auto& buffer = m_TinyglTFModel.buffers[bufferView.buffer];

			unsigned char* dataPtr = &buffer.data[0] + bufferView.byteOffset;
			m_Buffers.push_back(std::make_shared<Buffer>(BufferDesc(), accessor.count, accessor.ByteStride(bufferView), dataPtr));
		}
		else
		{
			ASSERT(false, "glTF model mesh does not contain attribute " + attributeName);
		}
	}

	// Make instance data buffer
	struct InstanceData
	{
		glm::mat4 transform = glm::identity<glm::mat4>();
		glm::vec4 color = glm::vec4(1.0f);
	} instanceData;

	instanceData.transform = glm::scale(instanceData.transform, glm::vec3(3.0f));
	instanceData.transform = glm::rotate(instanceData.transform, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	instanceData.transform = glm::rotate(instanceData.transform, glm::radians(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	instanceData.transform = glm::rotate(instanceData.transform, glm::radians(180.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	m_Buffers.push_back(std::make_shared<Buffer>(BufferDesc(), 1, 80, &instanceData));

	// Load indices from glTF model
	uint32_t indiciesIndex = m_TinyglTFModel.meshes[0].primitives[0].indices;
	auto& accessor = m_TinyglTFModel.accessors[indiciesIndex];
	auto& bufferView = m_TinyglTFModel.bufferViews[accessor.bufferView];
	auto& buffer = m_TinyglTFModel.buffers[bufferView.buffer];

	unsigned char* dataPtr = &m_TinyglTFModel.buffers[0].data[0] + bufferView.byteOffset;

	m_Buffers.push_back(std::make_shared<Buffer>(BufferDesc(), accessor.count, accessor.ByteStride(bufferView), dataPtr));
}

void Model::CreateTextures()
{
	auto& material = m_TinyglTFModel.materials[0];
	uint32_t albedoTextureIndex = material.pbrMetallicRoughness.baseColorTexture.index;
	uint32_t imageIndex = m_TinyglTFModel.textures[albedoTextureIndex].source;

	m_Textures.push_back(std::make_shared<Texture>(TextureDesc(DXGI_FORMAT_R8G8B8A8_UNORM, D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_FLAG_NONE,
		m_TinyglTFModel.images[imageIndex].width, m_TinyglTFModel.images[imageIndex].height), &m_TinyglTFModel.images[imageIndex].image[0]));
}
