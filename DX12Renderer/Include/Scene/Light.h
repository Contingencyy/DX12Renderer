#pragma once

struct Pointlight
{
	Pointlight() = default;
	Pointlight(const glm::vec3& position, float range, const glm::vec3& attenuation, const glm::vec4& diffuse, const glm::vec4& ambient)
		: Position(position), Range(range), Attenuation(attenuation), Diffuse(diffuse), Ambient(ambient) {}

	glm::vec3 Position = glm::vec3(0.0f);
	float Range = 1.0f;
	glm::vec3 Attenuation = glm::vec3(0.0f);
	BYTE_PADDING(4);
	glm::vec4 Diffuse = glm::vec4(1.0f);
	glm::vec4 Ambient = glm::vec4(0.0f);
};
