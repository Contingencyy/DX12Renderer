#pragma once
#include "Transform.h"

class ViewFrustum
{
public:
	ViewFrustum() = default;
	~ViewFrustum();

	void SetNearFarTangent(float near, float far, float tangent);

	void UpdateBounds(float aspectRatio);
	void UpdatePlanes(const Transform& transform);

	bool IsPointInViewFrustum(const glm::vec3& point) const;
	bool IsSphereInViewFrustum(const glm::vec3& point, float radius) const;
	bool IsBoxInViewFrustum(const glm::vec3& min, const glm::vec3& max) const;

	float GetNear() const { return m_Near; }
	float GetFar() const { return m_Far; }

private:
	struct Plane
	{
		Plane() = default;
		Plane(const glm::vec3& p0, const glm::vec3& normal)
			: Point(p0), Normal(normal) {}

		glm::vec3 Point = glm::vec3(0.0f);
		glm::vec3 Normal = glm::vec3(0.0f);
	};

	Plane m_Planes[6];

	float m_Near = 0.1f;
	float m_NearWidth = 1280.0f;
	float m_NearHeight = 720.0f;

	float m_Far = 1000.0f;
	float m_FarWidth = 1280.0f;
	float m_FarHeight = 720.0f;

	float m_Tangent = 0.0f;

};
