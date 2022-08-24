#pragma once
#include "Graphics/Buffer.h"
#include "Graphics/Texture.h"
#include "Scene/Camera/Camera.h"
#include "Scene/LightObject.h"

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
	static void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	static void Finalize();

	static void BeginFrame(const Camera& camera);
	static void Render();
	static void ImGuiRender();
	static void EndFrame();

	static void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
	static void Submit(const PointlightData& pointlightData);
	static void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);

	static void Resize(uint32_t width, uint32_t height);
	static void ToggleVSync();
	static bool IsVSyncEnabled();

	static const RenderSettings& GetSettings();

	// Temp
	static RenderPass& GetRenderPass();

private:
	static void CreateRenderPasses();
	static void InitializeBuffers();

};
