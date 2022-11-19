#pragma once
#include "Scene/Camera/Camera.h"
#include "Components/DirLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"

class Mesh;
class RenderPass;
class Buffer;
class Texture;
class CommandList;

class Renderer
{
public:
	struct RenderSettings
	{
		struct Resolution
		{
			uint32_t x = 1280;
			uint32_t y = 720;
		};

		Resolution RenderResolution;
		Resolution ShadowMapResolution = { 2048, 2048 };
		bool VSync = true;

		uint32_t MaxInstances = 1000;
		uint32_t MaxMeshes = 1000;
		uint32_t MaxDirectionalLights = 1;
		uint32_t MaxPointLights = 50;
		uint32_t MaxSpotLights = 50;
	};

public:
	static void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	static void Finalize();

	static void BeginScene(const Camera& sceneCamera, const glm::vec3& ambient);
	static void Render();
	static void OnImGuiRender();
	static void EndScene();

	static void Submit(const std::shared_ptr<Mesh>& mesh, const glm::mat4& transform);
	static void Submit(const DirectionalLightData& dirLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap);
	static void Submit(const PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, const std::shared_ptr<Texture>& shadowMap);
	static void Submit(const SpotLightData& spotLightData, const Camera& lightCamera, const std::shared_ptr<Texture>& shadowMap);

	static void Resize(uint32_t width, uint32_t height);
	static void ToggleVSync();
	static bool IsVSyncEnabled();

	static const RenderSettings& GetSettings();

	// Temp
	static Texture& GetFinalColorOutput();
	static Texture& GetFinalDepthOutput();

private:
	static void MakeRenderPasses();
	static void MakeBuffers();
	static void MakeFrameBuffers();

	static void PrepareInstanceBuffer();
	static void PrepareLightBuffers();

	static void RenderShadowMap(CommandList& commandList, const Camera& lightCamera, const Texture& shadowMap, uint32_t descriptorOffset = 0);

};
