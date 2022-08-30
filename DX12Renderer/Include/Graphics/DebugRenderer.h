#pragma once
#include "Scene/Camera/Camera.h"
#include "Graphics/Renderer.h"

class DebugRenderer
{
public:
	static void Initialize(uint32_t width, uint32_t height);
	static void Finalize();

	static void BeginScene(const Camera& sceneCamera, const glm::vec3& ambient);
	static void Render();
	static void OnImGuiRender();
	static void EndScene();

	static void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);

	static void Resize(uint32_t width, uint32_t height);

private:
	static void MakeRenderPasses();
	static void MakeBuffers();

private:
	struct DebugRendererSettings
	{
		bool Enable = true;
	};

	struct DebugRendererStatistics
	{
		uint32_t DrawCallCount;
		uint32_t LineCount;

		void Reset()
		{
			DrawCallCount = 0;
			LineCount = 0;
		}
	};

private:
	std::unique_ptr<RenderPass> m_RenderPass;

	DebugRendererSettings m_DebugRenderSettings;
	DebugRendererStatistics m_DebugRenderStatistics;

	D3D12_VIEWPORT m_Viewport = D3D12_VIEWPORT();
	D3D12_RECT m_ScissorRect = D3D12_RECT();

	struct LineVertex
	{
		LineVertex(const glm::vec3& position, const glm::vec4& color)
			: Position(position), Color(color) {}

		glm::vec3 Position;
		glm::vec4 Color;
	};

	std::vector<LineVertex> m_LineVertexData;
	std::unique_ptr<Buffer> m_LineBuffer;

	Renderer::SceneData m_SceneData;
	std::unique_ptr<Buffer> m_SceneDataConstantBuffer;

};
