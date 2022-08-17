#include "Pch.h"
#include "Camera.h"
#include "InputHandler.h"

#include "Application.h"
#include "Window.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

Camera::Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far)
	: m_FOV(fov)
{
	m_AspectRatio = width / height;

	m_ViewFrustum.Near = near;
	m_ViewFrustum.Far = far;
	m_ViewFrustum.Tangent = (float)glm::tan(glm::pi<float>() / 180.0f * m_FOV * 0.5f);

	UpdateViewFrustumBounds();

	m_Transform.Translate(pos);

	m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.Near, m_ViewFrustum.Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	MakeViewFrustumPlanes();
}

Camera::~Camera()
{
}

void Camera::Update(float deltaTime)
{
	bool translated = false;
	glm::vec3 velocity = glm::vec3(0.0f);

	velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * WorldForward;
	velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * WorldRight;
	velocity += InputHandler::GetInputAxis1D(KeyCode::SHIFT, KeyCode::CTRL) * WorldUp;

	if (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f)
	{
		velocity = glm::normalize(velocity);
		m_Transform.Translate(velocity * m_Speed * deltaTime);

		translated = true;
	}

	bool rotated = false;
	glm::vec3 rotation = glm::vec3(0.0f);

	if (InputHandler::IsKeyPressed(KeyCode::RIGHT_MOUSE))
	{
		glm::vec2 mouseRelPos = InputHandler::GetMousePositionAbs() -
			glm::vec2(Application::Get().GetWindow()->GetWidth() / 2, Application::Get().GetWindow()->GetHeight() / 2);
		float rotationStrength = glm::length(mouseRelPos);
		mouseRelPos = glm::normalize(mouseRelPos);

		if (rotationStrength > m_RotationDeadZone)
		{
			float rotationStrengthSqrt = sqrt(rotationStrength);

			rotation += WorldUp * mouseRelPos.x;
			rotation += WorldRight * mouseRelPos.y;
			rotation += WorldForward * InputHandler::GetInputAxis1D(KeyCode::Q, KeyCode::E);

			rotation *= m_RotationSpeed * rotationStrengthSqrt * deltaTime;
			m_Transform.Rotate(rotation);

			glm::vec3 currentRotation = m_Transform.GetRotation();
			printf("%f, %f, %f\n", currentRotation.x, currentRotation.y, currentRotation.z);

			rotated = true;
		}
	}
	
	if (translated || rotated)
	{
		m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		MakeViewFrustumPlanes();
	}
}

void Camera::ImGuiRender()
{
	ImGui::Checkbox("Frustum culling", &m_EnableFrustumCulling);
}

void Camera::ResizeProjection(float width, float height)
{
	m_ProjectionMatrix = glm::perspectiveFovLH_ZO(glm::radians(m_FOV), width, height, m_ViewFrustum.Near, m_ViewFrustum.Far);
	m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

	m_AspectRatio = width / height;

	UpdateViewFrustumBounds();
	MakeViewFrustumPlanes();
}

bool Camera::IsPointInViewFrustum(const glm::vec3& point) const
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

bool Camera::IsSphereInViewFrustum(const glm::vec3& point, float radius) const
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
	glm::vec3 cameraPosition = m_Transform.GetPosition();
	glm::vec3 cameraRight = m_Transform.Right();
	glm::vec3 cameraUp = m_Transform.Up();
	glm::vec3 cameraForward = m_Transform.Forward();

	glm::vec3 nearCenter = cameraPosition + cameraForward * m_ViewFrustum.Near;
	glm::vec3 farCenter = cameraPosition + cameraForward * m_ViewFrustum.Far;

	m_ViewFrustum.Planes[4] = { nearCenter, cameraForward };
	m_ViewFrustum.Planes[5] = { farCenter, -cameraForward };

	glm::vec3 aux = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);

	aux = glm::normalize((nearCenter + cameraUp * m_ViewFrustum.NearHeight) - cameraPosition);
	normal = glm::cross(cameraRight, aux);
	m_ViewFrustum.Planes[0] = { nearCenter + cameraUp * m_ViewFrustum.NearHeight, normal };

	aux = glm::normalize((nearCenter - cameraUp * m_ViewFrustum.NearHeight) - cameraPosition);
	normal = glm::cross(aux, cameraRight);
	m_ViewFrustum.Planes[1] = { nearCenter - cameraUp * m_ViewFrustum.NearHeight, normal };

	aux = glm::normalize((nearCenter - cameraRight * m_ViewFrustum.NearWidth) - cameraPosition);
	normal = glm::cross(cameraUp, aux);
	m_ViewFrustum.Planes[2] = { nearCenter - cameraRight * m_ViewFrustum.NearWidth, normal };

	aux = glm::normalize((nearCenter + cameraRight * m_ViewFrustum.NearWidth) - cameraPosition);
	normal = glm::cross(aux, cameraUp);
	m_ViewFrustum.Planes[3] = { nearCenter + cameraRight * m_ViewFrustum.NearWidth, normal };
}
