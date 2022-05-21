#pragma once

class Buffer
{
public:
	Buffer();
	virtual ~Buffer();

	virtual void CreateView() = 0;

	std::size_t GetAlignedSize() const;

	ComPtr<ID3D12Resource> GetD3D12Resource() const;
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource);

protected:
	ComPtr<ID3D12Resource> m_d3d12Resource;
	std::size_t m_AlignedBufferSize = 0;

};
