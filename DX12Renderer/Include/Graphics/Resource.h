#pragma once
#include "Graphics/Backend/DescriptorAllocation.h"

class Resource
{
public:
	Resource(const std::string& name);
	virtual ~Resource() = 0;

	// Could add pure virtuals here for setting/reading back data and invalidating (e.g. when resizing)
	// virtual void ReadData() = 0;
	// virtual void SetData(const void* data, std::size_t numBytes, std::size_t byteOffset = 0) = 0;
	// virtual void Invalidate() = 0;

	virtual bool IsValid() const = 0;
	virtual bool IsCPUAccessible() const = 0;
	virtual void Invalidate() = 0;

	const DescriptorAllocation& GetDescriptorAllocation(DescriptorType type) const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetDescriptor(DescriptorType type, uint32_t offset = 0) const;
	uint32_t GetDescriptorHeapIndex(DescriptorType type, uint32_t offset = 0);

	ComPtr<ID3D12Resource> GetD3D12Resource() const { return m_d3d12Resource; }
	void SetD3D12Resource(ComPtr<ID3D12Resource> resource) { m_d3d12Resource = resource; }
	const std::string& GetName() const { return m_Name; }
	void SetName(const std::string& name);
	std::size_t GetByteSize() const { return m_ByteSize; }

protected:
	virtual void CreateD3D12Resource() = 0;
	virtual void AllocateDescriptors() = 0;
	virtual void CreateViews() = 0;
	void ResetDescriptorAllocations();

protected:
	ComPtr<ID3D12Resource> m_d3d12Resource;
	DescriptorAllocation m_DescriptorAllocations[DescriptorType::NUM_DESCRIPTOR_TYPES] = {};

	std::string m_Name = "";
	std::size_t m_ByteSize = 0;

	void* m_CPUPtr = nullptr;

};
