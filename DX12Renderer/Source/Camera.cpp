#include "Pch.h"
#include "Camera.h"

Camera::Camera()
{
}

Camera::Camera(const glm::vec3& pos, const glm::vec3& targetPos, float fov, float width, float height, float near, float far)
	: m_FOV(fov), m_Near(near), m_Far(far)
{
	m_ViewMatrix = glm::lookAtLH(pos, targetPos, glm::vec3(0.0f, 1.0f, 0.0f));
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}

Camera::~Camera()
{
}

void Camera::Update(float deltaTime)
{
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
}
