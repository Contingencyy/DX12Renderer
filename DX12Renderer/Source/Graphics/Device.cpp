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
