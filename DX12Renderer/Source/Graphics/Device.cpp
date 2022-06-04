#include "Pch.h"
#include "Graphics/Device.h"

Device::Device()
{
    EnableDebugLayer();

	CreateAdapter();
	CreateDevice();
}

Device::~Device()
{
}

void Device::CreateCommandQueue(CommandQueue& commandQueue, const D3D12_COMMAND_QUEUE_DESC& queueDesc)
{
    ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
    DX_CALL(m_d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&d3d12CommandQueue)));
    commandQueue.SetD3D12CommandQueue(d3d12CommandQueue);

    ComPtr<ID3D12Fence> d3d12Fence;
    DX_CALL(m_d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&d3d12Fence)));
    commandQueue.SetD3D12Fence(d3d12Fence);
}

void Device::CreateCommandList(CommandList& commandList)
{
    ComPtr<ID3D12CommandAllocator> d3d12Allocator;
    DX_CALL(m_d3d12Device->CreateCommandAllocator(commandList.GetCommandListType(), IID_PPV_ARGS(&d3d12Allocator)));

    ComPtr<ID3D12GraphicsCommandList2> d3d12CommandList;
    DX_CALL(m_d3d12Device->CreateCommandList(0, commandList.GetCommandListType(), d3d12Allocator.Get(), nullptr, IID_PPV_ARGS(&d3d12CommandList)));

    commandList.SetD3D12CommandAllocator(d3d12Allocator);
    commandList.SetD3D12CommandList(d3d12CommandList);
}

void Device::CreateDescriptorHeap(const D3D12_DESCRIPTOR_HEAP_DESC& heapDesc, ComPtr<ID3D12DescriptorHeap>& heap)
{
    DX_CALL(m_d3d12Device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&heap)));
}

void Device::CreatePipelineState(const D3D12_GRAPHICS_PIPELINE_STATE_DESC& pipelineStateDesc, ComPtr<ID3D12PipelineState>& pipelineState)
{
    DX_CALL(m_d3d12Device->CreateGraphicsPipelineState(&pipelineStateDesc, IID_PPV_ARGS(&pipelineState)));
}

void Device::CreateRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC& rootSignatureDesc, ComPtr<ID3D12RootSignature>& rootSignature)
{
    ComPtr<ID3DBlob> serializedRootSig;
    ComPtr<ID3DBlob> errorBlob;
    DX_CALL(D3D12SerializeVersionedRootSignature(&rootSignatureDesc, &serializedRootSig, &errorBlob));

    DX_CALL(m_d3d12Device->CreateRootSignature(0, serializedRootSig->GetBufferPointer(),
        serializedRootSig->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));
}

void Device::CreateBuffer(Buffer& buffer, D3D12_HEAP_TYPE bufferType, D3D12_RESOURCE_STATES initialState, std::size_t size)
{
    CD3DX12_HEAP_PROPERTIES heapProps(bufferType);
    CD3DX12_RESOURCE_DESC heapDesc(CD3DX12_RESOURCE_DESC::Buffer(size));

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &heapDesc,
        initialState,
        nullptr,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    buffer.SetD3D12Resource(d3d12Resource);
}

void Device::CreateTexture(Texture& texture, const D3D12_RESOURCE_DESC& textureDesc, D3D12_RESOURCE_STATES initialState, const D3D12_CLEAR_VALUE* clearValue)
{
    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    ComPtr<ID3D12Resource> d3d12Resource;
    DX_CALL(m_d3d12Device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &textureDesc,
        initialState,
        clearValue,
        IID_PPV_ARGS(&d3d12Resource)
    ));

    texture.SetD3D12Resource(d3d12Resource);
}

void Device::CreateRenderTargetView(Texture& texture, const D3D12_RENDER_TARGET_VIEW_DESC& rtvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateRenderTargetView(texture.GetD3D12Resource().Get(), &rtvDesc, descriptor);
}

void Device::CreateDepthStencilView(Texture& texture, const D3D12_DEPTH_STENCIL_VIEW_DESC& dsvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateDepthStencilView(texture.GetD3D12Resource().Get(), &dsvDesc, descriptor);
}

void Device::CreateShaderResourceView(Texture& texture, const D3D12_SHADER_RESOURCE_VIEW_DESC& srvDesc, D3D12_CPU_DESCRIPTOR_HANDLE descriptor)
{
    m_d3d12Device->CreateShaderResourceView(texture.GetD3D12Resource().Get(), &srvDesc, descriptor);
}

uint32_t Device::GetDescriptorIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type)
{
    return m_d3d12Device->GetDescriptorHandleIncrementSize(type);
}

void Device::EnableDebugLayer()
{
#if defined(_DEBUG)
    ComPtr<ID3D12Debug> debugInterface;
    DX_CALL(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
    debugInterface->EnableDebugLayer();
#endif
}

void Device::CreateAdapter()
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
			DX_CALL(dxgiAdapter1.As(&m_dxgiAdapter));
		}
	}
}

void Device::CreateDevice()
{
    DX_CALL(D3D12CreateDevice(m_dxgiAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_d3d12Device)));

#if defined(_DEBUG)
    ComPtr<ID3D12InfoQueue> pInfoQueue;

    if (SUCCEEDED(m_d3d12Device.As(&pInfoQueue)))
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
            D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
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
