#include "Pch.h"
#include "Scene/Camera/ViewFrustum.h"

ViewFrustum::~ViewFrustum()
{
}

void ViewFrustum::SetNearFarTangent(float near, float far, float tangent)
{
	m_Near = near;
	m_Far = far;
	m_Tangent = tangent;
}

void ViewFrustum::UpdateBounds(float aspectRatio)
{
	m_NearHeight = m_Near * m_Tangent;
	m_NearWidth = m_NearHeight * aspectRatio;
	m_FarHeight = m_Far * m_Tangent;
	m_FarWidth = m_FarHeight * aspectRatio;
}

void ViewFrustum::UpdatePlanes(const Transform& transform)
{
	const glm::vec3& cameraPosition = transform.GetPosition();
	const Transform& invTransform = transform.GetInverse();
	const glm::vec3& cameraRight = invTransform.Right();
	const glm::vec3& cameraUp = invTransform.Up();
	const glm::vec3& cameraForward = invTransform.Forward();

	const glm::vec3& nearCenter = cameraPosition + cameraForward * m_Near;
	const glm::vec3& farCenter = cameraPosition + cameraForward * m_Far;

	// Near / far
	m_Planes[4] = { nearCenter, cameraForward };
	m_Planes[5] = { farCenter, -cameraForward };

	glm::vec3 aux = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);

	// Top
	aux = glm::normalize((nearCenter + cameraUp * m_NearHeight) - cameraPosition);
	normal = glm::cross(cameraRight, aux);
	m_Planes[0] = { nearCenter + cameraUp * m_NearHeight, normal };

	// Bottom
	aux = glm::normalize((nearCenter - cameraUp * m_NearHeight) - cameraPosition);
	normal = glm::cross(aux, cameraRight);
	m_Planes[1] = { nearCenter - cameraUp * m_NearHeight, normal };

	// Left
	aux = glm::normalize((nearCenter - cameraRight * m_NearWidth) - cameraPosition);
	normal = glm::cross(cameraUp, aux);
	m_Planes[2] = { nearCenter - cameraRight * m_NearWidth, normal };

	// Right
	aux = glm::normalize((nearCenter + cameraRight * m_NearWidth) - cameraPosition);
	normal = glm::cross(aux, cameraUp);
	m_Planes[3] = { nearCenter + cameraRight * m_NearWidth, normal };
}

bool ViewFrustum::IsPointInViewFrustum(const glm::vec3& point) const
{
	float distance = 0.0f;
	for (uint32_t i = 0; i < 6; ++i)
	{
		distance = glm::dot(m_Planes[i].Normal, (point - m_Planes[i].Point));
		if (distance < 0.0f)
			return false;
	}

	return true;
}

bool ViewFrustum::IsSphereInViewFrustum(const glm::vec3& point, float radius) const
{
	bool result = true;
	float distance = 0.0f;
	for (uint32_t i = 0; i < 6; ++i)
	{
		distance = glm::dot(m_Planes[i].Normal, (point - m_Planes[i].Point));
		if (distance < -radius)
			return false;
		else if (distance < radius)
			result = true;
	}

	return result;
}

bool ViewFrustum::IsBoxInViewFrustum(const glm::vec3& min, const glm::vec3& max) const
{
	bool result = true;

	for (uint32_t i = 0; i < 6; ++i)
	{
		const ViewFrustum::Plane& plane = m_Planes[i];

		glm::vec3 posVert = min;
		glm::vec3 negVert = max;

		if (plane.Normal.x >= 0)
		{
			posVert.x = max.x;
			negVert.x = min.x;
		}
		if (plane.Normal.y >= 0)
		{
			posVert.y = max.y;
			negVert.y = min.y;
		}
		if (plane.Normal.z >= 0)
		{
			posVert.z = max.z;
			negVert.z = min.z;
		}

		if (glm::dot(plane.Normal, (posVert - plane.Point)) < 0)
			return false;
		else if (glm::dot(plane.Normal, (negVert - plane.Point)) < 0)
			result = true;
	}

	return result;
}
