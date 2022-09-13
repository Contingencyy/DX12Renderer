#pragma once

class RootSignature
{
public:
	RootSignature(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters);
	~RootSignature();

	ComPtr<ID3D12RootSignature> GetD3D12RootSignature() const { return m_d3d12RootSignature; }

private:
	void Create(const std::string& name, const std::vector<CD3DX12_ROOT_PARAMETER1>& rootParameters);

private:
	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;

};
