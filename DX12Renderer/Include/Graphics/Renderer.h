#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Scene/Camera/Camera.h"
#include "Scene/LightObject.h"

class Device;
class CommandQueue;
class SwapChain;
class DescriptorHeap;
class Mesh;
class RenderPass;

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
		uint32_t MaxModelInstances = 100;
		uint32_t MaxPointLights = 100;
	};

	struct RenderStatistics
	{
		void Reset()
		{
			DrawCallCount = 0;
			TriangleCount = 0;
			MeshCount = 0;
			PointLightCount = 0;
		}

		uint32_t DrawCallCount = 0;
		uint32_t TriangleCount = 0;
		uint32_t MeshCount = 0;
		uint32_t PointLightCount = 0;
	};

public:
	Renderer();
	~Renderer();

	void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	void Finalize();

	void BeginFrame(const Camera& camera);
	void Render();
	void ImGuiRender();
	void EndFrame();

	void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
	void Submit(const PointlightData& pointlightData);
	void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);

	void Resize(uint32_t width, uint32_t height);
	void ToggleVSync() { m_RenderSettings.VSync = !m_RenderSettings.VSync; }
	bool IsVSyncEnabled() const { return m_RenderSettings.VSync; }

	const RenderSettings& GetSettings() { return m_RenderSettings; }

	// Temp
	RenderPass& GetRenderPass() const;

private:
	enum RenderPassType : uint32_t
	{
		DEFAULT,
		DEBUG_LINE,
		TONE_MAPPING,
		NUM_RENDER_PASSES = (TONE_MAPPING + 1)
	};

private:
	void CreateRenderPasses();
	void InitializeBuffers();

private:
	friend class GUI;

	std::unique_ptr<RenderPass> m_RenderPasses[RenderPassType::NUM_RENDER_PASSES];

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	RenderSettings m_RenderSettings;
	RenderStatistics m_RenderStatistics;

	struct MeshInstanceData
	{
		MeshInstanceData(const glm::mat4& transform, const glm::vec4& color)
			: Transform(transform), Color(color) {}

		glm::mat4 Transform = glm::identity<glm::mat4>();
		glm::vec4 Color = glm::vec4(1.0f);
	};
	
	struct MeshDrawData
	{
		MeshDrawData(const std::shared_ptr<Mesh>& mesh)
			: Mesh(mesh)
		{
		}

		std::shared_ptr<Mesh> Mesh;
		std::vector<MeshInstanceData> MeshInstanceData;
	};

	std::unordered_map<std::size_t, MeshDrawData> m_MeshDrawData;
	std::unique_ptr<Buffer> m_MeshInstanceBuffer;

	std::vector<PointlightData> m_PointlightDrawData;
	std::unique_ptr<Buffer> m_PointlightBuffer;

	struct LineVertex
	{
		LineVertex(const glm::vec3& position, const glm::vec4& color)
			: Position(position), Color(color) {}

		glm::vec3 Position;
		glm::vec4 Color;
	};

	std::vector<LineVertex> m_LineVertexData;
	std::unique_ptr<Buffer> m_LineBuffer;
	
	std::unique_ptr<Buffer> m_ToneMapVertexBuffer;
	std::unique_ptr<Buffer> m_ToneMapIndexBuffer;

	struct SceneData
	{
		SceneData() = default;

		glm::mat4 ViewProjection = glm::identity<glm::mat4>();
		glm::vec3 Ambient = glm::vec3(0.0f);
		uint32_t NumPointlights = 0;
	};

	SceneData m_SceneData;
	std::unique_ptr<Buffer> m_SceneDataConstantBuffer;

};
