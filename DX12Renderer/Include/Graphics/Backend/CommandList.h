#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/Backend/RenderBackend.h"

class DescriptorHeap;
class DynamicDescriptorHeap;
class Device;
class RenderPass;

class CommandList
{
public:
	CommandList(D3D12_COMMAND_LIST_TYPE type);
	~CommandList();

	void SetBackBufferAssociation(uint32_t backBufferIndex);

	void ClearRenderTargetView(D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float* clearColor);
	void ClearDepthStencilView(D3D12_CPU_DESCRIPTOR_HANDLE dsv, float depth = 1.0f);

	void SetViewports(uint32_t numViewports, D3D12_VIEWPORT* viewports);
	void SetScissorRects(uint32_t numRects, D3D12_RECT* rects);
	void SetRenderTargets(uint32_t numRTVS, D3D12_CPU_DESCRIPTOR_HANDLE* rtvs, D3D12_CPU_DESCRIPTOR_HANDLE* dsv);
	void SetRenderPassBindables(const RenderPass& renderPass);

	void SetRootConstants(uint32_t rootIndex, uint32_t numValues, const void* data, uint32_t offset);
	void SetVertexBuffers(uint32_t slot, uint32_t numViews, const Buffer& vertexBuffer);
	void SetIndexBuffer(const Buffer& indexBuffer);
	void SetDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, const DescriptorHeap& descriptorHeap);
	void SetRootDescriptorTable(uint32_t rootParameterIndex, D3D12_GPU_DESCRIPTOR_HANDLE baseDescriptor);

	void SetRootConstantBufferView(uint32_t rootParameterIndex, Buffer& buffer, D3D12_RESOURCE_STATES stateAfter);
	void SetRootShaderResourceView(uint32_t rootParameterIndex, Texture& texture, D3D12_RESOURCE_STATES stateAfter);

	void BeginTimestampQuery(const std::string& name);
	void EndTimestampQuery(const std::string& name);
	void ResolveTimestampQueries();
	
	void Draw(uint32_t vertexCount, uint32_t instanceCount, uint32_t startVertex = 0, uint32_t startInstance = 0);
	void DrawIndexed(uint32_t indexCount, uint32_t instanceCount, uint32_t startIndex = 0, int32_t baseVertex = 0, uint32_t startInstance = 0);

	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CopyBufferRegion(Buffer& intermediateBuffer, std::size_t intermediateOffset, Buffer& destBuffer, std::size_t destOffset, std::size_t numBytes);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);
	void ResolveTexture(Texture& srcTexture, Texture& destTexture);

	void Transition(Resource& resource, D3D12_RESOURCE_STATES stateAfter);
	void TrackObject(ComPtr<ID3D12Object> object);
	void ReleaseTrackedObjects();

	void Close();
	void Reset();

	void SetD3D12CommandAllocator(ComPtr<ID3D12CommandAllocator> d3d12CommandAllocator) { m_d3d12CommandAllocator = d3d12CommandAllocator; }
	void SetD3D12CommandList(ComPtr<ID3D12GraphicsCommandList2> d3d12CommandList) { m_d3d12CommandList = d3d12CommandList; }

	D3D12_COMMAND_LIST_TYPE GetCommandListType() const { return m_d3d12CommandListType; }
	ComPtr<ID3D12GraphicsCommandList2> GetGraphicsCommandList() const { return m_d3d12CommandList; }

private:
	ComPtr<ID3D12GraphicsCommandList2> m_d3d12CommandList;
	D3D12_COMMAND_LIST_TYPE m_d3d12CommandListType;
	ComPtr<ID3D12CommandAllocator> m_d3d12CommandAllocator;

	uint32_t m_BackBufferIndex = 0;

	std::vector<ComPtr<ID3D12Object>> m_TrackedObjects;

	ID3D12RootSignature* m_RootSignature;
	ID3D12DescriptorHeap* DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	std::unordered_map<std::string, TimestampQuery> m_TimestampQueries;

};
