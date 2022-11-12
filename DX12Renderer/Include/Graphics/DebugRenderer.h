#pragma once
#include "Scene/Camera/Camera.h"
#include "Graphics/Renderer.h"

class DebugRenderer
{
public:
	static void Initialize(uint32_t width, uint32_t height);
	static void Finalize();

	static void BeginScene(const Camera& sceneCamera);
	static void Render();
	static void OnImGuiRender();
	static void EndScene();

	static void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);

	static void Resize(uint32_t width, uint32_t height);

private:
	static void MakeRenderPasses();
	static void MakeBuffers();
	static void MakeFrameBuffers();
	

};
