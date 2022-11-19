#include "Pch.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/RenderBackend.h"
#include "Graphics/Backend/SwapChain.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority)
    : m_d3d12CommandListType(type)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = priority;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    auto d3d12Device = RenderBackend::GetD3D12Device();

    DX_CALL(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
    DX_CALL(d3d12Device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));
    m_d3d12CommandQueue->GetTimestampFrequency(&m_TimestampFrequency);
}

CommandQueue::~CommandQueue()
{
}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
    std::shared_ptr<CommandList> commandList;
    if (!m_AvailableCommandLists.Empty())
    {
        m_AvailableCommandLists.TryPop(commandList);
    }
    else
    {
        commandList = std::make_shared<CommandList>(m_d3d12CommandListType);
    }

    commandList->SetBackBufferAssociation(RenderBackend::GetSwapChain().GetCurrentBackBufferIndex());
    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
    commandList->Close();

    ID3D12CommandList* const ppCommandLists[] = { commandList->GetGraphicsCommandList().Get() };
    m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    uint64_t fenceValue = Signal();

    m_InFlightCommandLists.Push({ commandList, fenceValue });
    return fenceValue;
}

uint64_t CommandQueue::Signal()
{
    uint64_t fenceValue = ++m_FenceValue;
    m_d3d12CommandQueue->Signal(m_d3d12Fence.Get(), fenceValue);

    return fenceValue;
}

bool CommandQueue::IsFenceComplete() const
{
    return m_d3d12Fence->GetCompletedValue() >= m_FenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue) const
{
    if (!IsFenceComplete())
    {
        HANDLE fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
        ASSERT(fenceEvent, "Failed to creat efence event handle");

        DX_CALL(m_d3d12Fence->SetEventOnCompletion(fenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));

        ::CloseHandle(fenceEvent);
    }
}

void CommandQueue::ResetCommandLists()
{
    std::unique_lock<std::mutex> lock(m_InFlightCommandListsMutex, std::defer_lock);

    InFlightCommandList inFlightCommandList;
    lock.lock();

    while (m_InFlightCommandLists.TryPop(inFlightCommandList))
    {
        WaitForFenceValue(inFlightCommandList.FenceValue);
        inFlightCommandList.CommandList->Reset();
        m_AvailableCommandLists.Push(inFlightCommandList.CommandList);
    }

    lock.unlock();
    m_InFlightCommandListsCV.notify_one();
}

void CommandQueue::Flush()
{
    std::unique_lock<std::mutex> lock(m_InFlightCommandListsMutex);
    m_InFlightCommandListsCV.wait(lock, [this] { return m_InFlightCommandLists.Empty(); });

    WaitForFenceValue(m_FenceValue);
}
