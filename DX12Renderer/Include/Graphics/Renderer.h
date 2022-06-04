#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Camera.h"

class CommandQueue;
class SwapChain;
class Device;
class Model;

class Renderer
{
public:
	struct RenderSettings
	{
		struct Resolution
		{
			uint32_t x;
			uint32_t y;
		} Resolution;

		bool VSync = true;
	};

	struct RenderStatistics
	{
		void Reset()
		{
			DrawCallCount = 0;
			TriangleCount = 0;
		}

		uint32_t DrawCallCount = 0;
		uint32_t TriangleCount = 0;
	};

public:
	Renderer();
	~Renderer();

	void Initialize(uint32_t width, uint32_t height);
	void Finalize();

	void BeginFrame(const Camera& camera);
	void Render();
	void ImGuiRender();
	void EndFrame();

	void DrawQuads(std::shared_ptr<Buffer> instanceBuffer, std::shared_ptr<Texture> texture, std::size_t numQuads);
	void DrawModel(Model* model);
	void Resize(uint32_t width, uint32_t height);

	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);
	D3D12_CPU_DESCRIPTOR_HANDLE AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);

	void ToggleVSync() { m_RenderSettings.VSync = !m_RenderSettings.VSync; }
	bool IsVSyncEnabled() const { return m_RenderSettings.VSync; }

	std::shared_ptr<SwapChain> GetSwapChain() const { return m_SwapChain; }
	std::shared_ptr<CommandQueue> GetCommandQueueDirect() const { return m_CommandQueueDirect; }
	std::shared_ptr<Device> GetDevice() const { return m_Device; }

	RenderSettings GetRenderSettings() const { return m_RenderSettings; }
	RenderStatistics GetRenderStatistics() const { return m_RenderStatistics; }

private:
	void Flush();
	void CreateDescriptorHeap();

private:
	friend class GUI;

	std::shared_ptr<Device> m_Device;
	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::shared_ptr<CommandQueue> m_CommandQueueCopy;
	std::shared_ptr<SwapChain> m_SwapChain;

	ComPtr<ID3D12DescriptorHeap> m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorOffsets[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];
	uint32_t m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	RenderSettings m_RenderSettings;
	RenderStatistics m_RenderStatistics;
	
	std::unique_ptr<Buffer> m_QuadVertexBuffer;
	std::unique_ptr<Buffer> m_QuadIndexBuffer;

	struct QuadDrawData
	{
		std::shared_ptr<Buffer> InstanceBuffer = nullptr;
		std::shared_ptr<Texture> Texture = nullptr;
		std::size_t NumQuads = 1;
	};

	std::vector<QuadDrawData> m_QuadDrawData;
	std::vector<Model*> m_ModelDrawData;

	struct SceneData
	{
		SceneData() {}

		Camera Camera;
	};

	SceneData m_SceneData;

};
