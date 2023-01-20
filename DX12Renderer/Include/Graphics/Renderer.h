#pragma once

struct MeshPrimitive;
struct BoundingBox;
struct DirectionalLightData;
struct PointLightData;
struct SpotLightData;
struct RenderResourceHandle;
struct RenderSettings;
struct BufferDesc;
struct TextureDesc;

class Camera;

// Remove soon
class Texture;

namespace Renderer
{

	void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	void Finalize();

	void BeginScene(const Camera& sceneCamera);
	void Render();
	void OnImGuiRender();
	void EndScene();

	void Submit(const MeshPrimitive& meshPrimitive, const BoundingBox& bb, const glm::mat4& transform);
	void Submit(const DirectionalLightData& dirLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle);
	void Submit(const SpotLightData& spotLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle);
	void Submit(const PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, RenderResourceHandle shadowMapHandle);

	RenderResourceHandle CreateBuffer(const BufferDesc& desc);
	RenderResourceHandle CreateTexture(const TextureDesc& desc);

	void Resize(uint32_t width, uint32_t height);
	void ToggleVSync();
	bool IsVSyncEnabled();

	const RenderSettings& GetSettings();

};
