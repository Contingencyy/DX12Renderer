#include "Pch.h"
#include "Scene/Camera/Camera.h"
#include "InputHandler.h"
#include "Application.h"
#include "Window.h"

#include <imgui/imgui.h>

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_FOV(fov), m_Width(width), m_Height(height)
{
	m_AspectRatio = m_Width / m_Height;
	m_ReversedZ = (near > far);

	m_ViewFrustum.SetNearFarTangent(near, far, (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f));
	m_ViewFrustum.UpdateBounds(m_AspectRatio);

	m_Transform.Translate(pos);

	m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), m_Width, m_Height, near, far);

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_ViewFrustum.UpdatePlanes(m_Transform);
}

Camera::Camera(const glm::mat4& view, float fov, float aspect, float near, float far)
	: m_FOV(fov), m_Width(1.0f), m_Height(1.0f)
{
	m_AspectRatio = aspect;
	m_ReversedZ = (near > far);

	m_ViewFrustum.SetNearFarTangent(near, far, (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f));
	m_ViewFrustum.UpdateBounds(m_AspectRatio);

	m_Transform = Transform(glm::inverse(view));

	m_ViewMatrix = view;
	m_ProjectionMatrix = glm::perspectiveLH_ZO(glm::radians(m_FOV), m_AspectRatio, near, far);
	/*m_ProjectionMatrix[2][2] = 0.0f;
	m_ProjectionMatrix[3][2] = m_ViewFrustum.GetFar();*/

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_ViewFrustum.UpdatePlanes(m_Transform);
}

Camera::Camera(const glm::mat4& view, float left, float right, float bottom, float top, float near, float far)
	: m_EnableFrustumCulling(false)
{
	m_Transform = Transform(glm::inverse(view));
	m_ReversedZ = (near > far);

	m_ViewMatrix = view;
	m_ProjectionMatrix = glm::orthoLH_ZO(left, right, bottom, top, near, far);
	/*m_ProjectionMatrix[2][2] = 0.0f;
	m_ProjectionMatrix[3][2] = far;*/

	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
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
	m_Width = width;
	m_Height = height;

	m_AspectRatio = m_Width / m_Height;

	float near = m_ReversedZ ? m_ViewFrustum.GetFar() : m_ViewFrustum.GetNear();
	float far = m_ReversedZ ? m_ViewFrustum.GetNear() : m_ViewFrustum.GetFar();

	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), m_Width, m_Height, near, far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;
	
	m_ViewFrustum.UpdateBounds(m_AspectRatio);
	m_ViewFrustum.UpdatePlanes(m_Transform);
}

void Camera::SetViewMatrix(const glm::mat4& view)
{
	m_ViewMatrix = view;
	m_Transform = Transform(glm::inverse(view));
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_ViewFrustum.UpdatePlanes(m_Transform);
}

void Camera::OnImGuiRender()
{
	ImGui::Text("Camera");
	ImGui::Checkbox("Frustum culling", &m_EnableFrustumCulling);
	if (ImGui::DragFloat("FOV", &m_FOV, 1.0f, 1.0f, 150.0f, "%.0f"))
	{
		m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), m_Width, m_Height, m_ViewFrustum.GetNear(), m_ViewFrustum.GetFar());
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		m_ViewFrustum.SetNearFarTangent(m_ViewFrustum.GetNear(), m_ViewFrustum.GetFar(), (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f));
		m_ViewFrustum.UpdateBounds(m_AspectRatio);
		m_ViewFrustum.UpdatePlanes(m_Transform);
	}
	ImGui::DragFloat("Exposure", &m_Exposure, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat("Gamma", &m_Gamma, 0.01f, 0.01f, 10.0f);
	ImGui::DragFloat("Speed", &m_Speed, 0.1f, 0.01f, 10000.0f);
	if (ImGui::Button("Debug print transform"))
	{
		glm::mat4 mat = m_Transform.GetTransformMatrix();
		LOG_INFO("Camera transform: " + glm::to_string(mat));
	}
}

bool Camera::UpdateMovement(float deltaTime)
{
	glm::vec3 velocity = glm::vec3(0.0f);
	const Transform& invTransform = m_Transform.GetInverse();

	velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * invTransform.Forward();
	velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * invTransform.Right();
	velocity += InputHandler::GetInputAxis1D(KeyCode::SPACEBAR, KeyCode::CTRL) * invTransform.Up();

	float speedMultiplier = InputHandler::IsKeyPressed(KeyCode::SHIFT) ? 5.0f : 1.0f;

	if (glm::length(velocity) > 0.0f)
	{
		velocity = glm::normalize(velocity);
		m_Transform.Translate(velocity * m_Speed * speedMultiplier * deltaTime);

		return true;
	}

	return false;
}

bool Camera::UpdateRotation(float deltaTime)
{
	if (InputHandler::IsKeyPressed(KeyCode::RIGHT_MOUSE))
	{
		if (m_SetAnchorPointOnClick)
		{
			m_RotationAnchorPoint = InputHandler::GetMousePositionAbs();
			m_SetAnchorPointOnClick = false;
		}

		glm::vec2 mouseDelta = InputHandler::GetMousePositionAbs() - m_RotationAnchorPoint;
		float mouseDeltaLength = glm::length(mouseDelta) - m_RotationDeadZone;

		if (mouseDeltaLength > 0)
		{
			glm::vec2 mouseDirectionFromCenter = glm::normalize(mouseDelta);
			float rotationStrength = sqrt(sqrt(mouseDeltaLength));

			float yawSign = m_Transform.Up().y < 0.0f ? -1.0f : 1.0f;
			m_Yaw += yawSign * mouseDirectionFromCenter.x * m_RotationSpeed * rotationStrength * deltaTime;
			m_Pitch += mouseDirectionFromCenter.y * m_RotationSpeed * rotationStrength * deltaTime;

			m_Pitch = std::min(m_Pitch, glm::radians(90.0f));
			m_Pitch = std::max(m_Pitch, glm::radians(-90.0f));

			m_Transform.SetRotation(glm::vec3(m_Pitch, m_Yaw, 0.0f));

			return true;
		}
	}
	else if (!m_SetAnchorPointOnClick)
	{
		m_SetAnchorPointOnClick = true;
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
