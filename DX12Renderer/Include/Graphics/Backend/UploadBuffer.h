#pragma once

struct UploadBufferAllocation
{
	ID3D12Resource* D3D12Resource;
	std::size_t Size;
	std::size_t OffsetInBuffer;
};

class UploadBuffer
{
public:
	UploadBuffer(std::size_t totalByteSize);
	~UploadBuffer();

	UploadBufferAllocation Allocate(std::size_t byteSize, std::size_t align = 0);

private:
	ComPtr<ID3D12Resource> m_d3d12Resource;
	void* m_CPUPtr = nullptr;

	std::size_t m_TotalByteSize = 0;
	std::size_t m_CurrentByteOffset = 0;

};
