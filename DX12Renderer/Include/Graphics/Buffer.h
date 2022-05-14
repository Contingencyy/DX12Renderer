#pragma once

class Buffer
{
public:
	explicit Buffer();
	virtual ~Buffer();

	virtual void CreateView(std::size_t numElements, std::size_t elementSize) = 0;

	ComPtr<ID3D12Resource> GetD3D12Resource() const;
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource);

protected:
	ComPtr<ID3D12Resource> m_d3d12Resource;

};
