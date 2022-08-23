#include "Pch.h"
#include "Scene/Camera/Camera.h"
#include "InputHandler.h"

#include "Application.h"
#include "Graphics/Renderer.h"
#include "Window.h"

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_FOV(fov)
{
	m_AspectRatio = width / height;

	m_ViewFrustum.SetNearFarTangent(near, far, (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f));
	m_ViewFrustum.UpdateBounds(m_AspectRatio);

	m_Transform.Translate(pos);

	m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.GetNear(), m_ViewFrustum.GetFar());
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_ViewFrustum.UpdatePlanes(m_Transform);
}

Camera::~Camera()
{
}

void Camera::Update(float deltaTime)
{
	bool translated = UpdateMovement(deltaTime);
	bool rotated = UpdateRotation(deltaTime);

	if (translated || rotated)
	{
		m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ViewFrustum.UpdatePlanes(m_Transform);
	}
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.GetNear(), m_ViewFrustum.GetFar());
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_AspectRatio = width / height;

	m_ViewFrustum.UpdateBounds(m_AspectRatio);
	m_ViewFrustum.UpdatePlanes(m_Transform);
}

bool Camera::UpdateMovement(float deltaTime)
{
	glm::vec3 velocity = glm::vec3(0.0f);
	const Transform& invTransform = m_Transform.GetInverse();

	velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * invTransform.Forward();
	velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * invTransform.Right();
	velocity += InputHandler::GetInputAxis1D(KeyCode::SHIFT, KeyCode::CTRL) * invTransform.Up();

	if (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f)
	{
		velocity = glm::normalize(velocity);
		m_Transform.Translate(velocity * m_Speed * deltaTime);

		return true;
	}

	return false;
}

bool Camera::UpdateRotation(float deltaTime)
{
	if (InputHandler::IsKeyPressed(KeyCode::RIGHT_MOUSE))
	{
		glm::vec2 screenCenter(Application::Get().GetWindow()->GetWidth() / 2, Application::Get().GetWindow()->GetHeight() / 2);
		glm::vec2 mouseDelta = InputHandler::GetMousePositionAbs() - screenCenter;
		float mouseDeltaLength = glm::length(mouseDelta);

		if (mouseDeltaLength > m_RotationDeadZone)
		{
			glm::vec2 mouseDirectionFromCenter = glm::normalize(mouseDelta);
			float rotationStrength = sqrt(sqrt(mouseDeltaLength));

			float yawSign = m_Transform.Up().y < 0.0f ? -1.0f : 1.0f;
			m_Yaw += yawSign * mouseDirectionFromCenter.x * m_RotationSpeed * rotationStrength * deltaTime;
			m_Pitch += mouseDirectionFromCenter.y * m_RotationSpeed * rotationStrength * deltaTime;

			m_Transform.SetRotation(glm::vec3(m_Pitch, m_Yaw, 0.0f));

			return true;
		}
	}

	/*if (InputHandler::IsKeyPressed(KeyCode::Q) || InputHandler::IsKeyPressed(KeyCode::E))
	{
		float rollInputAxis = InputHandler::GetInputAxis1D(KeyCode::Q, KeyCode::E);
		glm::vec3 rotation(0.0f);
		rotation.z = rollInputAxis;

		m_Transform.Rotate(rotation);
		rotated = true;
	}*/

	return false;
}
