#pragma once
#include "Component.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/RenderAPI.h"

struct PointLightData
{
	PointLightData() = default;
	PointLightData(const glm::vec3& attenuation, const glm::vec3& color)
		: Attenuation(attenuation), Color(color) {}

	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 0.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Color = glm::vec3(1.0f);
	uint32_t ShadowMapIndex = 0;
};

class PointLightComponent : public Component
{
public:
	PointLightComponent(const PointLightData& pointLightData);
	~PointLightComponent();

	virtual void Update(float deltaTime);
	virtual void Render();
	virtual void OnImGuiRender();
	
private:
	PointLightData m_PointLightData;
	std::array<Camera, 6> m_Cameras;
	RenderResourceHandle m_ShadowMap;

};
