#pragma once

struct BoundingBox
{
	glm::vec3 Min = glm::vec3(0.0f);
	glm::vec3 Max = glm::vec3(0.0f);

};

struct BoundingSphere
{
	glm::vec3 Position = glm::vec3(0.0f);
	float Radius = 0.5f;

};
