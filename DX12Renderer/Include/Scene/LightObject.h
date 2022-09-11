#pragma once
#include "Scene/SceneObject.h"

struct DirectionalLightData
{
	DirectionalLightData() = default;
	DirectionalLightData(const glm::vec3& direction, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Direction(direction), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Direction = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Ambient = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Diffuse = glm::vec3(1.0f);
	BYTE_PADDING(1);
};

class DirectionalLightObject : public SceneObject
{
public:
	DirectionalLightObject(const DirectionalLightData& dirLightData, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~DirectionalLightObject();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera);

private:
	DirectionalLightData m_DirectionalLightData;

};

struct PointLightData
{
	PointLightData() = default;
	PointLightData(const glm::vec3& attenuation, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Attenuation(attenuation), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Position = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Attenuation = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Ambient = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Diffuse = glm::vec3(1.0f);
	BYTE_PADDING(1);
};

class PointLightObject : public SceneObject
{
public:
	PointLightObject(const PointLightData& pointLightData, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~PointLightObject();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera);

private:
	PointLightData m_PointLightData;

};

struct SpotLightData
{
	SpotLightData() = default;
	SpotLightData(const glm::vec3& direction, const glm::vec3& attenuation, float innerConeAngle, float outerConeAngle, const glm::vec3& ambient, const glm::vec3& diffuse)
		: Direction(direction), Attenuation(attenuation), InnerConeAngle(glm::cos(glm::radians(innerConeAngle))), OuterConeAngle(glm::cos(glm::radians(outerConeAngle))), Ambient(ambient), Diffuse(diffuse) {}

	glm::vec3 Position = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Direction = glm::vec3(0.0f);
	BYTE_PADDING(1);
	glm::vec3 Attenuation = glm::vec3(0.0f);
	float InnerConeAngle = 0.0f;
	float OuterConeAngle = 0.0f;
	glm::vec3 Ambient = glm::vec3(0.0f);
	glm::vec3 Diffuse = glm::vec3(0.0f);
	BYTE_PADDING(1);
};

class SpotLightObject : public SceneObject
{
public:
	SpotLightObject(const SpotLightData& spotLightData, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~SpotLightObject();

	virtual void Update(float deltaTime);
	virtual void Render(const Camera& camera);

private:
	SpotLightData m_SpotLightData;

};
