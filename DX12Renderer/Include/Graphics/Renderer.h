#pragma once

struct MeshPrimitive;
struct BoundingBox;
struct DirectionalLightData;
struct PointLightData;
struct SpotLightData;
struct RenderResourceHandle;
struct BufferDesc;
struct TextureDesc;
struct MeshDesc;
struct MaterialDesc;
struct Resolution;

class Camera;

namespace Renderer
{

	void Initialize(HWND hWnd, uint32_t width, uint32_t height);
	void Finalize();

	void BeginFrame();
	void BeginScene(const Camera& sceneCamera);
	void Render();
	void OnImGuiRender();
	void EndScene();
	void EndFrame();

	void Submit(RenderResourceHandle meshPrimitiveHandle, const glm::mat4& transform, const glm::mat4& prevFrameTransform);
	void Submit(DirectionalLightData& dirLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle);
	void Submit(SpotLightData& spotLightData, const Camera& lightCamera, RenderResourceHandle shadowMapHandle);
	void Submit(PointLightData& pointLightData, const std::array<Camera, 6>& lightCameras, RenderResourceHandle shadowMapHandle);

	RenderResourceHandle CreateBuffer(const BufferDesc& desc);
	RenderResourceHandle CreateTexture(const TextureDesc& desc);
	RenderResourceHandle CreateMesh(const MeshDesc& desc);
	RenderResourceHandle CreateMaterial(const MaterialDesc& desc);

	void Resize(uint32_t width, uint32_t height);
	void ToggleVSync();
	bool IsVSyncEnabled();

	const Resolution& GetRenderResolution();
	const Resolution& GetShadowResolution();

};
