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

	// Draws a line between lineStart and lineEnd
	void Submit(const glm::vec3& lineStart, const glm::vec3& lineEnd, const glm::vec4& color);
	// Draws a box with its min and max bounds
	void SubmitAABB(const glm::vec3& min, const glm::vec3& max, const glm::vec4& color);

};
