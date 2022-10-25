#pragma once
#include "Component.h"

class Texture;

struct DirectionalLightData
{
	DirectionalLightData() = default;
	DirectionalLightData(const glm::vec3& direction, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Direction(direction), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Direction = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Ambient = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Diffuse = glm::vec3(1.0f);
	BYTE_PADDING(4);

	glm::mat4 ViewProjection = glm::identity<glm::mat4>();
	uint32_t ShadowMapIndex = 0;
	BYTE_PADDING(12);
};

class DirLightComponent : public Component
{
public:
	DirLightComponent(const DirectionalLightData& dirLightData);
	virtual ~DirLightComponent();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera, const Transform& transform);
	virtual void OnImGuiRender();

private:
	DirectionalLightData m_DirectionalLightData;
	std::shared_ptr<Texture> m_ShadowMap;

};
