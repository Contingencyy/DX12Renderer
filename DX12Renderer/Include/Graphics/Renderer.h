#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Graphics/DescriptorAllocation.h"
#include "Camera.h"
#include "Scene/Light.h"

class Device;
class CommandQueue;
class SwapChain;
class DescriptorHeap;
class Model;

class Renderer
{
public:
	struct RenderSettings
	{
		struct Resolution
		{
			uint32_t x = 1280;
			uint32_t y = 720;
		} Resolution;

		bool VSync = true;
		uint32_t MaxModelInstances = 1000;
		uint32_t MaxPointLights = 100;
	};

	struct RenderStatistics
	{
		void Reset()
		{
			DrawCallCount = 0;
			TriangleCount = 0;
			ObjectCount = 0;
			PointLightCount = 0;
		}

		uint32_t DrawCallCount = 0;
		uint32_t TriangleCount = 0;
		uint32_t ObjectCount = 0;
		uint32_t PointLightCount = 0;
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

	void Submit(const std::shared_ptr<Model>& model, const glm::mat4& transform);
	void Submit(const Pointlight& pointlight);
	void Resize(uint32_t width, uint32_t height);

	void CopyBuffer(Buffer& intermediateBuffer, Buffer& destBuffer, const void* bufferData);
	void CopyTexture(Buffer& intermediateBuffer, Texture& destTexture, const void* textureData);
	DescriptorAllocation AllocateDescriptors(uint32_t numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type);

	void ToggleVSync() { m_RenderSettings.VSync = !m_RenderSettings.VSync; }
	bool IsVSyncEnabled() const { return m_RenderSettings.VSync; }

	std::shared_ptr<SwapChain> GetSwapChain() const { return m_SwapChain; }
	std::shared_ptr<CommandQueue> GetCommandQueueDirect() const { return m_CommandQueueDirect; }
	std::shared_ptr<Device> GetDevice() const { return m_Device; }

	RenderSettings& GetRenderSettings() { return m_RenderSettings; }
	const RenderSettings& GetRenderSettings() const { return m_RenderSettings; }
	RenderStatistics& GetRenderStatistics() { return m_RenderStatistics; }
	const RenderStatistics& GetRenderStatistics() const { return m_RenderStatistics; }

private:
	void Flush();

private:
	friend class GUI;

	std::shared_ptr<Device> m_Device;
	std::shared_ptr<CommandQueue> m_CommandQueueDirect;
	std::shared_ptr<CommandQueue> m_CommandQueueCopy;
	std::shared_ptr<SwapChain> m_SwapChain;
	std::shared_ptr<DescriptorHeap> m_DescriptorHeaps[D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES];

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	RenderSettings m_RenderSettings;
	RenderStatistics m_RenderStatistics;

	struct ModelInstanceData
	{
		ModelInstanceData(const glm::mat4& transform, const glm::vec4& color)
			: Transform(transform), Color(color) {}

		glm::mat4 Transform = glm::identity<glm::mat4>();
		glm::vec4 Color = glm::vec4(1.0f);
	};
	
	struct ModelDrawData
	{
		ModelDrawData(const std::shared_ptr<Model>& model)
			: Model(model)
		{
		}

		std::shared_ptr<Model> Model;
		std::vector<ModelInstanceData> ModelInstanceData;
	};

	std::vector<ModelDrawData> m_ModelDrawData;
	std::unique_ptr<Buffer> m_ModelInstanceBuffer;

	std::unique_ptr<Buffer> m_PointlightBuffer;
	std::vector<Pointlight> m_Pointlights;

	struct SceneData
	{
		SceneData() = default;

		Camera Camera;
	};

	SceneData m_SceneData;

};
