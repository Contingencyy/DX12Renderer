#include "Pch.h"
#include "Camera.h"
#include "InputHandler.h"

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_Position(pos), m_FOV(fov)
{
	m_AspectRatio = width / height;

	m_ViewFrustum.Near = near;
	m_ViewFrustum.Far = far;
	m_ViewFrustum.Tangent = (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f);

	UpdateViewFrustumBounds();

	m_ViewMatrix = glm::inverse(glm::translate(glm::identity<glm::mat4>(), m_Position));
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.Near, m_ViewFrustum.Far);

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	MakeViewFrustumPlanes();
}

Camera::~Camera()
{
}

void Camera::Update(float deltaTime)
{
	m_Velocity = glm::vec3(0.0f);

	m_Velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * Forward();
	m_Velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * Right();
	m_Velocity += InputHandler::GetInputAxis1D(KeyCode::Q, KeyCode::E) * Up();

	if (m_Velocity.x != 0.0f || m_Velocity.y != 0.0f || m_Velocity.z != 0.0f)
	{
		m_Velocity = glm::normalize(m_Velocity);
		m_Position += m_Velocity * m_Speed * deltaTime;

		m_ViewMatrix = glm::inverse(glm::translate(glm::identity<glm::mat4>(), m_Position));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		MakeViewFrustumPlanes();
	}
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.Near, m_ViewFrustum.Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_AspectRatio = width / height;

	UpdateViewFrustumBounds();
	MakeViewFrustumPlanes();
}

bool Camera::IsPointInViewFrustum(const glm::vec3& point)
{
	float distance = 0.0f;
	for (uint32_t i = 0; i < 6; ++i)
	{
		distance = glm::dot(m_ViewFrustum.Planes[i].Normal, (point - m_ViewFrustum.Planes[i].Point));
		if (distance < 0.0f)
			return false; // Point lies outside of view frustum
	}

	return true; // Point lies inside of view frustum
}

bool Camera::IsSphereInViewFrustum(const glm::vec3& point, float radius)
{
	bool result = true;
	float distance = 0.0f;
	for (uint32_t i = 0; i < 6; ++i)
	{
		distance = glm::dot(m_ViewFrustum.Planes[i].Normal, (point - m_ViewFrustum.Planes[i].Point));
		if (distance < -radius)
			return false; // Sphere lies outside of view frustum
		else if (distance < radius)
			result = true; // Sphere intersects with view frustum
	}

	return result; // Sphere lies inside of view frustum
}

void Camera::UpdateViewFrustumBounds()
{
	m_ViewFrustum.NearHeight = m_ViewFrustum.Near * m_ViewFrustum.Tangent;
	m_ViewFrustum.NearWidth = m_ViewFrustum.NearHeight * m_AspectRatio;
	m_ViewFrustum.FarHeight = m_ViewFrustum.Far * m_ViewFrustum.Tangent;
	m_ViewFrustum.FarWidth = m_ViewFrustum.FarHeight * m_AspectRatio;
}

void Camera::MakeViewFrustumPlanes()
{
	glm::vec3 nearCenter = m_Position + Forward() * m_ViewFrustum.Near;
	glm::vec3 farCenter = m_Position + Forward() * m_ViewFrustum.Far;

	m_ViewFrustum.Planes[4] = { nearCenter, Forward() };
	m_ViewFrustum.Planes[5] = { farCenter, -Forward() };

	glm::vec3 aux = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);

	aux = glm::normalize((nearCenter + Up() * m_ViewFrustum.NearHeight) - m_Position);
	normal = glm::cross(Right(), aux);
	m_ViewFrustum.Planes[0] = { nearCenter + Up() * m_ViewFrustum.NearHeight, normal };

	aux = glm::normalize((nearCenter - Up() * m_ViewFrustum.NearHeight) - m_Position);
	normal = glm::cross(aux, Right());
	m_ViewFrustum.Planes[1] = { nearCenter - Up() * m_ViewFrustum.NearHeight, normal };

	aux = glm::normalize((nearCenter - Right() * m_ViewFrustum.NearWidth) - m_Position);
	normal = glm::cross(Up(), aux);
	m_ViewFrustum.Planes[2] = { nearCenter - Right() * m_ViewFrustum.NearWidth, normal };

	aux = glm::normalize((nearCenter + Right() * m_ViewFrustum.NearWidth) - m_Position);
	normal = glm::cross(aux, Up());
	m_ViewFrustum.Planes[3] = { nearCenter + Right() * m_ViewFrustum.NearWidth, normal };
}

glm::vec3 Camera::Forward() const
{
	glm::vec4 forward = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
	return glm::vec3(forward);
}

glm::vec3 Camera::Right() const
{
	glm::vec4 right = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f);
	return glm::vec3(right);
}

glm::vec3 Camera::Up() const
{
	glm::vec4 up = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f);
	return glm::vec3(up);
}
