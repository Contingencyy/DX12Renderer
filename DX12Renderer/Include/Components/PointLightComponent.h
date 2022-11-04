#pragma once
#include "Component.h"
#include "Scene/Camera/Camera.h"

class Texture;

struct PointLightData
{
	PointLightData() = default;
	PointLightData(const glm::vec3& attenuation, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Attenuation(attenuation), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 0.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Ambient = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec3 Diffuse = glm::vec3(1.0f);
	uint32_t ShadowMapIndex = 0;
};

class PointLightComponent : public Component
{
public:
	PointLightComponent(const PointLightData& pointLightData, const glm::vec3& position);
	~PointLightComponent();

	virtual void Update(float deltaTime);
	virtual void Render(const Transform& transform);
	virtual void OnImGuiRender();
	
private:
	PointLightData m_PointLightData;
	std::array<Camera, 6> m_Cameras;
	std::shared_ptr<Texture> m_ShadowMap;

};
