#pragma once

struct DescriptorTableRange
{
	DescriptorTableRange(uint32_t rootParameterIndex, uint32_t offset, uint32_t numDescriptors)
		: RootParameterIndex(rootParameterIndex), Offset(offset), NumDescriptors(numDescriptors) {}

	uint32_t RootParameterIndex;
	uint32_t Offset;
	uint32_t NumDescriptors;
};

class RootSignature
{
public:
	RootSignature();
	~RootSignature();

	const std::vector<DescriptorTableRange>& GetDescriptorTableRanges() const { return m_DescriptorTableRanges; }
	const std::vector<DescriptorTableRange>& GetSamplerTableRanges() const { return m_SamplerTableRanges; }

	ComPtr<ID3D12RootSignature> GetD3D12RootSignature() const { return m_d3d12RootSignature; }

private:
	void Create();
	void ParseDescriptorTableRanges(const D3D12_ROOT_SIGNATURE_DESC1& rootSignatureDesc);

private:
	ComPtr<ID3D12RootSignature> m_d3d12RootSignature;

	std::vector<DescriptorTableRange> m_DescriptorTableRanges;
	std::vector<DescriptorTableRange> m_SamplerTableRanges;

};
