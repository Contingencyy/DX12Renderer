#pragma once
#include "Scene/SceneObject.h"

struct PointlightData
{
	PointlightData() = default;
	PointlightData(float range, const glm::vec3& attenuation, const glm::vec4& diffuse, const glm::vec4& ambient)
		: Range(range), Attenuation(attenuation), Diffuse(diffuse), Ambient(ambient) {}

	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 1.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec4 Diffuse = glm::vec4(1.0f);
	glm::vec4 Ambient = glm::vec4(0.0f);
};

class PointlightObject : public SceneObject
{
public:
	PointlightObject(const PointlightData& pointlightData, const std::string& name,
		const glm::vec3& translation = glm::vec3(0.0f), const glm::vec3& rotation = glm::vec3(0.0f), const glm::vec3& scale = glm::vec3(1.0f));
	~PointlightObject();

	virtual void Update(float deltaTime);
	virtual void Render();

private:
	PointlightData m_PointlightData;

};
