#include "Pch.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/DescriptorHeap.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/UploadBuffer.h"
#include "Graphics/Shader.h"
#include "Graphics/RenderState.h"

#include <imgui/imgui.h>

struct InternalRenderBackendData
{
	ComPtr<IDXGIAdapter4> DXGIAdapter4;
	ComPtr<ID3D12Device2> D3D12Device2;

	DXGI_QUERY_VIDEO_MEMORY_INFO DXGIQueryVideoMemoryInfo;

	std::unique_ptr<SwapChain> SwapChain;
	std::unique_ptr<DescriptorHeap> DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	ComPtr<ID3D12QueryHeap> D3D12QueryHeapTimestamp;
	std::unique_ptr<Buffer> QueryReadbackBuffers[3];
	uint32_t MaxQueriesPerFrame = 32;
	uint32_t NextQueryIndex[3] = { 0, 0, 0 };

	std::shared_ptr<CommandQueue> CommandQueueDirect;
	std::unique_ptr<CommandQueue> CommandQueueCompute;
	std::unique_ptr<CommandQueue> CommandQueueCopy;

	std::unique_ptr<UploadBuffer> UploadBuffer;

	ComPtr<ID3D12PipelineState> MipMapGenPSO;
	ComPtr<ID3D12RootSignature> MipMapGenRootSig;
	std::unique_ptr<Shader> MipMapGenShader;

	std::thread ProcessInFlightCommandListsThread;
	std::atomic_bool ProcessInFlightCommandLists;

	std::unordered_map<std::string, TimestampQuery> TimestampQueries[3];
	uint32_t CurrentBackBufferIndex = 0;

	bool VSync = true;
};

static InternalRenderBackendData s_Data;

void EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> d3d12DebugController0;
	ComPtr<ID3D12Debug1> d3d12DebugController1;

	DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&d3d12DebugController0)));
	d3d12DebugController0->EnableDebugLayer();

	if (GPU_VALIDATION_ENABLED)
	{
		DX_CALL(d3d12DebugController0->QueryInterface(IID_PPV_ARGS(&d3d12DebugController1)));
		d3d12DebugController1->SetEnableGPUBasedValidation(true);
		LOG_INFO("[Device] Enabled GPU based validation");
	}
#endif
}

void CreateAdapter()
{
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;

#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	DX_CALL(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));

	ComPtr<IDXGIAdapter1> dxgiAdapter1;

	SIZE_T maxDedicatedVideoMemory = 0;
	for (UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
	{
		DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
		dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

		if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
			SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0,
				__uuidof(ID3D12Device), nullptr)) && dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
		{
			maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
			DX_CALL(dxgiAdapter1.As(&s_Data.DXGIAdapter4));
		}
	}
}

void CreateDevice()
{
	DX_CALL(D3D12CreateDevice(s_Data.DXGIAdapter4.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&s_Data.D3D12Device2)));

#if defined(_DEBUG)
	if (GPU_VALIDATION_ENABLED)
	{
		// Set up GPU validation
		ComPtr<ID3D12DebugDevice1> d3d12DebugDevice;
		DX_CALL(s_Data.D3D12Device2->QueryInterface(IID_PPV_ARGS(&d3d12DebugDevice)));

		D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS debugValidationSettings = {};
		debugValidationSettings.MaxMessagesPerCommandList = 10;
		debugValidationSettings.DefaultShaderPatchMode = D3D12_GPU_BASED_VALIDATION_SHADER_PATCH_MODE_GUARDED_VALIDATION;
		debugValidationSettings.PipelineStateCreateFlags = D3D12_GPU_BASED_VALIDATION_PIPELINE_STATE_CREATE_FLAG_FRONT_LOAD_CREATE_GUARDED_VALIDATION_SHADERS;
		DX_CALL(d3d12DebugDevice->SetDebugParameter(D3D12_DEBUG_DEVICE_PARAMETER_GPU_BASED_VALIDATION_SETTINGS, &debugValidationSettings, sizeof(D3D12_DEBUG_DEVICE_GPU_BASED_VALIDATION_SETTINGS)));
	}

	// Set up info queue with filters
	ComPtr<ID3D12InfoQueue> pInfoQueue;

	if (SUCCEEDED(s_Data.D3D12Device2.As(&pInfoQueue)))
	{
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		// Suppress whole categories of messages
		//D3D12_MESSAGE_CATEGORY categories[] = {};

		// Suppress messages based on their severity level
		D3D12_MESSAGE_SEVERITY severities[] =
		{
			D3D12_MESSAGE_SEVERITY_INFO
		};

		// Suppress individual messages by their ID
		D3D12_MESSAGE_ID denyIds[] =
		{
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE,
			// This is a temporary fix for Windows 11 due to a bug that has not been fixed yet
			D3D12_MESSAGE_ID_RESOURCE_BARRIER_MISMATCHING_COMMAND_LIST_TYPE,
		};

		D3D12_INFO_QUEUE_FILTER newFilter = {};
		//newFilter.DenyList.NumCategories = _countof(categories);
		//newFilter.DenyList.pCategoryList = categories;
		newFilter.DenyList.NumSeverities = _countof(severities);
		newFilter.DenyList.pSeverityList = severities;
		newFilter.DenyList.NumIDs = _countof(denyIds);
		newFilter.DenyList.pIDList = denyIds;

		DX_CALL(pInfoQueue->PushStorageFilter(&newFilter));
	}
#endif
}

void CreateMipMapComputeState()
{
	// Root signature
	CD3DX12_DESCRIPTOR_RANGE1 ranges[2] = {};
	ranges[0].Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);
	ranges[1].Init(D3D12_DESCRIPTOR_RANGE_TYPE_UAV, 4096, 0, 0, D3D12_DESCRIPTOR_RANGE_FLAG_DESCRIPTORS_VOLATILE | D3D12_DESCRIPTOR_RANGE_FLAG_DATA_VOLATILE, 0);

	CD3DX12_ROOT_PARAMETER1 rootParameters[3] = {};
	rootParameters[0].InitAsConstants(8, 0);
	rootParameters[1].InitAsDescriptorTable(1, &ranges[0]);
	rootParameters[2].InitAsDescriptorTable(1, &ranges[1]);

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags = D3D12_ROOT_SIGNATURE_FLAG_NONE;

	// Default texture sampler (antisotropic)
	CD3DX12_STATIC_SAMPLER_DESC staticSamplers[1] = {};
	staticSamplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	staticSamplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_CLAMP;
	staticSamplers[0].MaxAnisotropy = 0;
	staticSamplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	staticSamplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
	staticSamplers[0].MinLOD = 0.0f;
	staticSamplers[0].MaxLOD = D3D12_FLOAT32_MAX;
	staticSamplers[0].MipLODBias = 0;
	staticSamplers[0].ShaderRegister = 0;
	staticSamplers[0].RegisterSpace = 0;
	staticSamplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC versionedRootSignatureDesc = {};
	versionedRootSignatureDesc.Init_1_1(_countof(rootParameters), &rootParameters[0], _countof(staticSamplers), &staticSamplers[0], rootSignatureFlags);

	ComPtr<ID3DBlob> serializedRootSig;
	ComPtr<ID3DBlob> errorBlob;

	HRESULT hr = D3D12SerializeVersionedRootSignature(&versionedRootSignatureDesc, &serializedRootSig, &errorBlob);
	if (!SUCCEEDED(hr) || errorBlob)
	{
		LOG_ERR(static_cast<const char*>(errorBlob->GetBufferPointer()));
		errorBlob->Release();
	}

	DX_CALL(s_Data.D3D12Device2->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
		serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&s_Data.MipMapGenRootSig)));
	s_Data.MipMapGenRootSig->SetName(StringHelper::StringToWString("Mip map root signature").c_str());

	// Shader
	s_Data.MipMapGenShader = std::make_unique<Shader>(L"Resources/Shaders/MipMapGen_CS.hlsl", "main", "cs_6_0");

	// Pipeline state
	D3D12_COMPUTE_PIPELINE_STATE_DESC psoDesc = {};
	psoDesc.pRootSignature = s_Data.MipMapGenRootSig.Get();
	psoDesc.CS = s_Data.MipMapGenShader->GetShaderByteCode();
	psoDesc.NodeMask = 0;
	psoDesc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;

	DX_CALL(s_Data.D3D12Device2->CreateComputePipelineState(&psoDesc, IID_PPV_ARGS(&s_Data.MipMapGenPSO)));
}

void ProcessTimestampQueries()
{
	uint32_t nextFrameBufferIndex = (s_Data.CurrentBackBufferIndex + 1) % RenderState::BACK_BUFFER_COUNT;

	for (auto& [name, timestampQuery] : s_Data.TimestampQueries[nextFrameBufferIndex])
	{
		timestampQuery.ReadFromBuffer(*s_Data.QueryReadbackBuffers[nextFrameBufferIndex]);

		TimerResult timer = {};
		timer.Name = name.c_str();
		timer.Duration = (timestampQuery.EndQueryTimestamp - timestampQuery.BeginQueryTimestamp) / (double)s_Data.CommandQueueDirect->GetTimestampFrequency() * 1000.0f;

		Profiler::AddGPUTimer(timer);
	}
}

void QueryVideoMemoryInfo()
{
	s_Data.DXGIAdapter4->QueryVideoMemoryInfo(0, DXGI_MEMORY_SEGMENT_GROUP_LOCAL, &s_Data.DXGIQueryVideoMemoryInfo);
}

void RenderBackend::Initialize(HWND hWnd, uint32_t width, uint32_t height)
{
	EnableDebugLayer();
	CreateAdapter();
	CreateDevice();

	uint32_t numDescriptorsPerHeap[4] = { 4096, 1, 512, 512 };
	for (uint32_t i = 0; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i)
		s_Data.DescriptorHeaps[i] = std::make_unique<DescriptorHeap>(static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>(i), numDescriptorsPerHeap[i]);

	s_Data.CommandQueueDirect = std::make_shared<CommandQueue>(D3D12_COMMAND_LIST_TYPE_DIRECT);
	s_Data.CommandQueueCompute = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COMPUTE);
	s_Data.CommandQueueCopy = std::make_unique<CommandQueue>(D3D12_COMMAND_LIST_TYPE_COPY);

	s_Data.UploadBuffer = std::make_unique<UploadBuffer>(MEGABYTE(100));
	s_Data.SwapChain = std::make_unique<SwapChain>(hWnd, s_Data.CommandQueueDirect, width, height);

	D3D12_QUERY_HEAP_DESC queryHeapDesc = {};
	queryHeapDesc.Type = D3D12_QUERY_HEAP_TYPE_TIMESTAMP;
	queryHeapDesc.Count = s_Data.MaxQueriesPerFrame * RenderState::BACK_BUFFER_COUNT;
	queryHeapDesc.NodeMask = 0;
	DX_CALL(s_Data.D3D12Device2->CreateQueryHeap(&queryHeapDesc, IID_PPV_ARGS(&s_Data.D3D12QueryHeapTimestamp)));

	for (uint32_t i = 0; i < RenderState::BACK_BUFFER_COUNT; ++i)
	{
		BufferDesc queryReadbackDesc = {};
		queryReadbackDesc.Usage = BufferUsage::BUFFER_USAGE_READBACK;
		queryReadbackDesc.NumElements = s_Data.MaxQueriesPerFrame;
		queryReadbackDesc.ElementSize = 8;
		queryReadbackDesc.DebugName = "Query result readback buffer";

		s_Data.QueryReadbackBuffers[i] = std::make_unique<Buffer>(queryReadbackDesc);
	}

	CreateMipMapComputeState();

	s_Data.ProcessInFlightCommandLists = true;
	s_Data.ProcessInFlightCommandListsThread = std::thread([]() {
		while (s_Data.ProcessInFlightCommandLists)
		{
			s_Data.CommandQueueDirect->ResetCommandLists();
			s_Data.CommandQueueCompute->ResetCommandLists();
			s_Data.CommandQueueCopy->ResetCommandLists();

			std::this_thread::yield();
		}
		});
}

void RenderBackend::BeginFrame()
{
	QueryVideoMemoryInfo();

	s_Data.TimestampQueries[s_Data.CurrentBackBufferIndex].clear();
	s_Data.NextQueryIndex[s_Data.CurrentBackBufferIndex] = 0;
}

void RenderBackend::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("GPU Memory Usage"))
	{
		ImGui::Indent(10.0f);

		ImGui::Text("Usage: %u MB", (uint64_t)(TO_MEGABYTE(s_Data.DXGIQueryVideoMemoryInfo.CurrentUsage)));
		ImGui::Text("Total budget: %u MB", (uint64_t)(TO_MEGABYTE(s_Data.DXGIQueryVideoMemoryInfo.Budget)));
		ImGui::Text("Reserved: %u MB", (uint64_t)(TO_MEGABYTE(s_Data.DXGIQueryVideoMemoryInfo.CurrentReservation)));
		ImGui::Text("Available reserve: %u MB", (uint64_t)(TO_MEGABYTE(s_Data.DXGIQueryVideoMemoryInfo.AvailableForReservation)));

		ImGui::Unindent(10.0f);
	}
}

void RenderBackend::EndFrame()
{
	s_Data.SwapChain->SwapBuffers(s_Data.VSync);
	s_Data.CurrentBackBufferIndex = s_Data.SwapChain->GetCurrentBackBufferIndex();

	ProcessTimestampQueries();
}

void RenderBackend::Finalize()
{
	Flush();

	s_Data.ProcessInFlightCommandLists = false;

	if (s_Data.ProcessInFlightCommandListsThread.joinable())
		s_Data.ProcessInFlightCommandListsThread.join();
}

void RenderBackend::CreateBuffer(ComPtr<ID3D12Resource>& d3d12Resource, D3D12_HEAP_TYPE heapType, const D3D12_RESOURCE_DESC& bufferDesc, D3D12_RESOURCE_STATES initialState)
{
	CD3DX12_HEAP_PROPERTIES heapProps(heapType);
	DX_CALL(s_Data.D3D12Device2->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		initialState,
		nullptr,
		IID_PPV_ARGS(&d3d12Resource)
	));
}

void RenderBackend::CreateTexture(ComPtr<ID3D12Resource>& d3d12Resource, const D3D12_RESOURCE_DESC& resourceDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
	CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);
	DX_CALL(s_Data.D3D12Device2->CreateCommittedResource(
		&heapProps,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		initialState,
		clearValue,
		IID_PPV_ARGS(&d3d12Resource)
	));
}

void RenderBackend::UploadBufferData(Buffer& destBuffer, const void* bufferData)
{
	UploadBufferAllocation upload = s_Data.UploadBuffer->Allocate(destBuffer.GetByteSize());

	auto commandList = s_Data.CommandQueueCopy->GetCommandList();
	commandList->CopyBuffer(upload, destBuffer, bufferData);
	uint64_t fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::UploadBufferDataRegion(Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes)
{
	UploadBufferAllocation upload = s_Data.UploadBuffer->Allocate(destBuffer.GetByteSize());

	auto commandList = s_Data.CommandQueueCopy->GetCommandList();
	commandList->CopyBufferRegion(upload, destBuffer, destOffset, numBytes);
	uint64_t fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
}

void RenderBackend::UploadTextureData(Texture& destTexture, const void* textureData)
{
	UploadBufferAllocation upload = s_Data.UploadBuffer->Allocate(destTexture.GetByteSize(), 512);

	auto copyCommandList = s_Data.CommandQueueCopy->GetCommandList();
	copyCommandList->CopyTexture(upload, destTexture, textureData);
	uint64_t copyFenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(copyCommandList);
	s_Data.CommandQueueCopy->WaitForFenceValue(copyFenceValue);
}

void RenderBackend::GenerateMips(Texture& texture)
{
	auto computeCommandList = s_Data.CommandQueueCompute->GetCommandList();
	computeCommandList->GenerateMips(texture);
	uint64_t computeFenceValue = s_Data.CommandQueueCompute->ExecuteCommandList(computeCommandList);
	s_Data.CommandQueueCompute->WaitForFenceValue(computeFenceValue);
}

ID3D12QueryHeap* RenderBackend::GetD3D12TimestampQueryHeap()
{
	return s_Data.D3D12QueryHeapTimestamp.Get();
}

const Buffer& RenderBackend::GetQueryReadbackBuffer(uint32_t backBufferIndex)
{
	return *s_Data.QueryReadbackBuffers[backBufferIndex];
}

void RenderBackend::TrackTimestampQueryResult(const std::string& name, const TimestampQuery& timestampQuery, uint32_t backBufferIndex)
{
	s_Data.TimestampQueries[backBufferIndex].emplace(name, timestampQuery);
	ASSERT(s_Data.TimestampQueries[backBufferIndex].size() <= s_Data.MaxQueriesPerFrame / 2, "Exceeded the maximum amount of queries per frame");
}

uint32_t RenderBackend::GetMaxQueriesPerFrame()
{
	return s_Data.MaxQueriesPerFrame;
}

uint32_t RenderBackend::GetNextQueryIndex(uint32_t backBufferIndex)
{
	return s_Data.NextQueryIndex[backBufferIndex]++;
}

void RenderBackend::Resize(uint32_t width, uint32_t height)
{
	width = std::max(1u, width);
	height = std::max(1u, height);

	Flush();

	s_Data.SwapChain->Resize(width, height);
}

void RenderBackend::Flush()
{
	s_Data.CommandQueueDirect->Flush();
	s_Data.CommandQueueCompute->Flush();
	s_Data.CommandQueueCopy->Flush();
}

void RenderBackend::SetVSync(bool vSync)
{
	s_Data.VSync = vSync;
}

IDXGIAdapter4* RenderBackend::GetDXGIAdapter()
{
	return s_Data.DXGIAdapter4.Get();
}

ID3D12Device2* RenderBackend::GetD3D12Device()
{
	return s_Data.D3D12Device2.Get();
}

SwapChain& RenderBackend::GetSwapChain()
{
	return *s_Data.SwapChain.get();
}

ID3D12PipelineState* RenderBackend::GetMipGenPSO()
{
	return s_Data.MipMapGenPSO.Get();
}

ID3D12RootSignature* RenderBackend::GetMipGenRootSig()
{
	return s_Data.MipMapGenRootSig.Get();
}

DescriptorAllocation RenderBackend::AllocateDescriptors(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	return s_Data.DescriptorHeaps[type]->Allocate(numDescriptors);
}

DescriptorHeap& RenderBackend::GetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
	return *s_Data.DescriptorHeaps[type].get();
}

std::shared_ptr<CommandList> RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE type)
{
	switch (type)
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		return s_Data.CommandQueueDirect->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		return s_Data.CommandQueueCompute->GetCommandList();
	case D3D12_COMMAND_LIST_TYPE_COPY:
		return s_Data.CommandQueueCopy->GetCommandList();
	}

	ASSERT(false, "Tried to retrieve command list from a command queue type that is not supported.");
	return nullptr;
}

void RenderBackend::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		s_Data.CommandQueueDirect->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		s_Data.CommandQueueCompute->ExecuteCommandList(commandList);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}

void RenderBackend::ExecuteCommandListAndWait(std::shared_ptr<CommandList> commandList)
{
	uint64_t fenceValue = 0;

	switch (commandList->GetCommandListType())
	{
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		fenceValue = s_Data.CommandQueueDirect->ExecuteCommandList(commandList);
		s_Data.CommandQueueDirect->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		fenceValue = s_Data.CommandQueueCompute->ExecuteCommandList(commandList);
		s_Data.CommandQueueCompute->WaitForFenceValue(fenceValue);
		return;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		fenceValue = s_Data.CommandQueueCopy->ExecuteCommandList(commandList);
		s_Data.CommandQueueCopy->WaitForFenceValue(fenceValue);
		return;
	}

	ASSERT(false, "Tried to execute command list on a command queue type that is not supported.");
}
