#pragma once
#include "Component.h"

class Texture;

struct SpotLightData
{
	SpotLightData() = default;
	SpotLightData(const glm::vec3& direction, const glm::vec3& attenuation, float innerConeAngle, float outerConeAngle, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Direction(direction), Attenuation(attenuation), InnerConeAngle(glm::cos(glm::radians(innerConeAngle))), OuterConeAngle(glm::cos(glm::radians(outerConeAngle))), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Position = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Direction = glm::vec3(0.0f);
	float Range = 0.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 0.0f;
	glm::vec3 Ambient = glm::vec3(0.0f);
	glm::vec3 Diffuse = glm::vec3(0.0f);
	BYTE_PADDING(4);

	glm::mat4 ViewProjection = glm::identity<glm::mat4>();
	uint32_t ShadowMapIndex = 0;
	BYTE_PADDING(12);
};

class SpotLightComponent : public Component
{
public:
	SpotLightComponent(const SpotLightData& spotLightData, const glm::vec3& position);
	~SpotLightComponent();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera, const Transform& transform);
	virtual void OnImGuiRender();

private:
	SpotLightData m_SpotLightData;
	std::shared_ptr<Texture> m_ShadowMap;
};
