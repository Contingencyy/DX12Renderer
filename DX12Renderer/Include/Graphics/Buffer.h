#pragma once

struct BufferDesc
{
	BufferDesc() = default;
	BufferDesc(D3D12_HEAP_TYPE type, D3D12_RESOURCE_STATES initialState)
		: Type(type), InitialState(initialState) {}

	D3D12_HEAP_TYPE Type = D3D12_HEAP_TYPE_DEFAULT;
	D3D12_RESOURCE_STATES InitialState = D3D12_RESOURCE_STATE_COMMON;
};

class Buffer
{
public:
	Buffer(const BufferDesc& bufferdesc, std::size_t numElements, std::size_t elementSize, const void* data);
	Buffer(const BufferDesc& bufferDesc, std::size_t numElements, std::size_t elementSize);
	Buffer(const BufferDesc& bufferDesc, std::size_t alignedSize);
	~Buffer();

	// This should not be called every frame, since it allocates another heap/buffer for upload in this function
	// Might be a good idea to have a larger upload heap on the command list to suballocate from for copying/staging.
	void SetBufferData(const void* data, uint32_t numElements = 0);
	void SetBufferDataStaging(const void* data, std::size_t alignedSize = 0);

	BufferDesc GetBufferDesc() const { return m_BufferDesc; }
	std::size_t GetByteSize() const { return m_ByteSize; }
	std::size_t GetNumElements() const { return m_NumElements; }
	std::size_t GetElementSize() const { return m_ElementSize; }

	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }

private:
	void Create();

private:
	BufferDesc m_BufferDesc = {};
	ComPtr<ID3D12Resource> m_d3d12Resource;

	void* m_CPUPtr = nullptr;

	std::size_t m_NumElements = 0;
	std::size_t m_ElementSize = 0;
	std::size_t m_ByteSize = 0;

};
