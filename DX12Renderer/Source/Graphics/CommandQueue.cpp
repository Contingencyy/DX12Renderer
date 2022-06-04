#include "Pch.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"

CommandQueue::CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority)
    : m_d3d12CommandListType(type)
{
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Type = type;
    queueDesc.Priority = priority;
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.NodeMask = 0;

    auto d3d12Device = Application::Get().GetRenderer()->GetDevice()->GetD3D12Device();

    DX_CALL(d3d12Device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&m_d3d12CommandQueue)));
    DX_CALL(d3d12Device->CreateFence(m_FenceValue, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&m_d3d12Fence)));
}

CommandQueue::~CommandQueue()
{
}

std::shared_ptr<CommandList> CommandQueue::GetCommandList()
{
    std::shared_ptr<CommandList> commandList;
    if (!m_AvailableCommandLists.empty())
    {
        commandList = m_AvailableCommandLists.front();
        m_AvailableCommandLists.pop();
    }
    else
    {
        commandList = std::make_shared<CommandList>(m_d3d12CommandListType);
    }

    return commandList;
}

uint64_t CommandQueue::ExecuteCommandList(std::shared_ptr<CommandList> commandList)
{
    commandList->Close();

    ID3D12CommandList* const ppCommandLists[] = { commandList->GetGraphicsCommandList().Get() };
    m_d3d12CommandQueue->ExecuteCommandLists(1, ppCommandLists);
    uint64_t fenceValue = Signal();

    m_InFlightCommandLists.push({ commandList, fenceValue });
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

        DX_CALL(m_d3d12Fence->SetEventOnCompletion(m_FenceValue, fenceEvent));
        ::WaitForSingleObject(fenceEvent, static_cast<DWORD>(std::chrono::milliseconds::max().count()));

        ::CloseHandle(fenceEvent);
    }
}

void CommandQueue::ResetCommandLists()
{
    std::shared_ptr<CommandList> commandList;
    while (!m_InFlightCommandLists.empty())
    {
        InFlightCommandList inFlight = m_InFlightCommandLists.front();
        m_InFlightCommandLists.pop();

        WaitForFenceValue(inFlight.FenceValue);
        inFlight.CommandList->Reset();

        m_AvailableCommandLists.push(inFlight.CommandList);
    }
}

void CommandQueue::Flush()
{
    WaitForFenceValue(m_FenceValue);
}
