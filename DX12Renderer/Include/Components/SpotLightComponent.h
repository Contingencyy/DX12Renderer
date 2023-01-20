#pragma once
#include "Component.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/RenderAPI.h"

struct SpotLightData
{
	SpotLightData() = default;
	SpotLightData(const glm::vec3& attenuation, float innerConeAngle, float outerConeAngle, const glm::vec3& color)
		: Attenuation(attenuation), InnerConeAngle(glm::cos(glm::radians(innerConeAngle))), OuterConeAngle(glm::cos(glm::radians(outerConeAngle))), Color(color) {}

	glm::vec3 Position = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Direction = glm::vec3(0.0f);
	float Range = 0.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 0.0f;
	glm::vec3 Color = glm::vec3(0.0f);

	glm::mat4 ViewProjection = glm::identity<glm::mat4>();
	uint32_t ShadowMapIndex = 0;
	BYTE_PADDING(12);
};

class SpotLightComponent : public Component
{
public:
	SpotLightComponent(const SpotLightData& spotLightData);
	~SpotLightComponent();

	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void OnImGuiRender();

private:
	struct GUIDataRepresentation
	{
		float InnerConeAngle = 0.0f;
		float OuterConeAngle = 0.0f;
	};

private:
	GUIDataRepresentation m_GUIData;

	SpotLightData m_SpotLightData;
	Camera m_Camera;
	RenderResourceHandle m_ShadowMap;
};
