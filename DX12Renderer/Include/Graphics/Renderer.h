#pragma once
#include "Scene/Camera/Camera.h"
#include "Scene/LightObject.h"

class Mesh;
class RenderPass;
class Buffer;
class Texture;

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

	struct SceneData
	{
		SceneData() = default;

		glm::mat4 ViewProjection = glm::identity<glm::mat4>();
		glm::vec3 Ambient = glm::vec3(0.0f);
		uint32_t NumPointlights = 0;
	};

public:
	static void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	static void Finalize();

	static void BeginFrame(const Camera& camera);
	static void Render();
	static void ImGuiRender();
	static void EndFrame();

	static void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
	static void Submit(const PointlightData& pointlightData);

	static void Resize(uint32_t width, uint32_t height);
	static void ToggleVSync();
	static bool IsVSyncEnabled();

	static const RenderSettings& GetSettings();

	// Temp
	static Texture& GetFinalColorOutput();
	// Temp
	static Texture& GetFinalDepthOutput();

private:
	static void MakeRenderPasses();
	static void MakeBuffers();

private:
	struct RendererStatistics
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

private:
	enum RenderPassType : uint32_t
	{
		DEFAULT,
		TONE_MAPPING,
		NUM_RENDER_PASSES = (TONE_MAPPING + 1)
	};

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

private:
	std::unique_ptr<RenderPass> m_RenderPasses[RenderPassType::NUM_RENDER_PASSES];

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	Renderer::RenderSettings m_RenderSettings;
	Renderer::RendererStatistics m_RenderStatistics;

	std::unordered_map<std::size_t, MeshDrawData> m_MeshDrawData = std::unordered_map<std::size_t, MeshDrawData>();
	std::unique_ptr<Buffer> m_MeshInstanceBuffer;

	std::vector<PointlightData> m_PointlightDrawData;
	std::unique_ptr<Buffer> m_PointlightBuffer;

	std::unique_ptr<Buffer> m_ToneMapVertexBuffer;
	std::unique_ptr<Buffer> m_ToneMapIndexBuffer;

	Renderer::SceneData m_SceneData;
	std::unique_ptr<Buffer> m_SceneDataConstantBuffer;

};
