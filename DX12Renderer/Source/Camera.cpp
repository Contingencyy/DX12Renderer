#include "Pch.h"
#include "Camera.h"
#include "InputHandler.h"

Camera::Camera()
{
}

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_Position(pos), m_FOV(fov), m_Near(near), m_Far(far)
{
	m_ViewMatrix = glm::inverse(glm::translate(glm::identity<glm::mat4>(), m_Position));
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
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
		m_Position += m_Velocity * deltaTime;

		m_ViewMatrix = glm::inverse(glm::translate(glm::identity<glm::mat4>(), m_Position));
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	}
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_Near, m_Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
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
