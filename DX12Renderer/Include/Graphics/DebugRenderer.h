#pragma once
#include "Scene/Camera/Camera.h"

namespace DebugRenderer
{

	void Initialize(uint32_t width, uint32_t height);
	void Finalize();

	void BeginScene(const Camera& sceneCamera);
	void Render();
	void OnImGuiRender();
	void EndScene();

	void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);

};
