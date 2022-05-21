#pragma once

class CommandList;

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

	ComPtr<ID3D12CommandQueue> GetD3D12CommandQueue() const { return m_d3d12CommandQueue; }

private:
	ComPtr<ID3D12CommandQueue> m_d3d12CommandQueue;
	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType = D3D12_COMMAND_LIST_TYPE_DIRECT;

	ComPtr<ID3D12Fence> m_d3d12Fence;
	uint64_t m_FenceValue = 0;

	struct InFlightCommandList
	{
		std::shared_ptr<CommandList> CommandList;
		uint64_t FenceValue;
	};

	std::queue<InFlightCommandList> m_InFlightCommandLists;
	std::queue<std::shared_ptr<CommandList>> m_AvailableCommandLists;

};
