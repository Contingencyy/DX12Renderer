#pragma once

class CommandList;
class Device;

class CommandQueue
{
public:
	CommandQueue(D3D12_COMMAND_LIST_TYPE type, D3D12_COMMAND_QUEUE_PRIORITY priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL);
	~CommandQueue();

	std::shared_ptr<CommandList> GetCommandList();
	uint64_t ExecuteCommandList(std::shared_ptr<CommandList> commandList);

	uint64_t Signal();
	bool IsFenceComplete() const;
	void WaitForFenceValue(uint64_t fenceValue) const;
	void ResetCommandLists();

	void Flush();

	void SetD3D12CommandQueue(ComPtr<ID3D12CommandQueue> d3d12CommandQueue) { m_d3d12CommandQueue = d3d12CommandQueue; }
	void SetD3D12Fence(ComPtr<ID3D12Fence> d3d12Fence) { m_d3d12Fence = d3d12Fence; }

	ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const { return m_d3d12CommandQueue; }
	uint64_t GetTimestampFrequency() const { return m_TimestampFrequency; }

private:
	ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;
	uint64_t m_TimestampFrequency = 0;

	ComPtr<ID3D12Fence> m_d3d12Fence;
	uint64_t m_FenceValue = 0;

	struct InFlightCommandList
	{
		std::shared_ptr<CommandList> CommandList;
		uint64_t FenceValue;
	};

	ThreadSafeQueue<InFlightCommandList> m_InFlightCommandLists;
	ThreadSafeQueue<std::shared_ptr<CommandList>> m_AvailableCommandLists;

	std::mutex m_InFlightCommandListsMutex;
	std::condition_variable m_InFlightCommandListsCV;

};
