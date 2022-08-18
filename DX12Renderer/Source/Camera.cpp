#include "Pch.h"
#include "Camera.h"
#include "InputHandler.h"

#include "Application.h"
#include "Graphics/Renderer.h"
#include "Window.h"

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
	const Transform& invTransform = m_Transform.GetInverse();

	velocity += InputHandler::GetInputAxis1D(KeyCode::W, KeyCode::S) * invTransform.Forward();
	velocity += InputHandler::GetInputAxis1D(KeyCode::D, KeyCode::A) * invTransform.Right();
	velocity += InputHandler::GetInputAxis1D(KeyCode::SHIFT, KeyCode::CTRL) * invTransform.Up();

	if (velocity.x != 0.0f || velocity.y != 0.0f || velocity.z != 0.0f)
	{
		velocity = glm::normalize(velocity);
		m_Transform.Translate(velocity * m_Speed * deltaTime);

		translated = true;
	}

	bool rotated = false;

	if (InputHandler::IsKeyPressed(KeyCode::RIGHT_MOUSE))
	{
		glm::vec2 mouseDelta = InputHandler::GetMousePositionAbs() -
			glm::vec2(Application::Get().GetWindow()->GetWidth() / 2, Application::Get().GetWindow()->GetHeight() / 2);
		float mouseDeltaWeight = glm::length(mouseDelta) - m_RotationDeadZone;

		if (mouseDeltaWeight > 0)
		{
			float rotationWeight = sqrt(mouseDeltaWeight);
			glm::vec2 normalizedMouseDelta = glm::normalize(mouseDelta);

			glm::vec3 rotation(0.0f);
			rotation += WorldUp * normalizedMouseDelta.x;
			rotation += WorldRight * normalizedMouseDelta.y;

			rotation *= m_RotationSpeed * rotationWeight * deltaTime;
			m_Transform.Rotate(rotation);
			rotated = true;
		}
	}

	if (InputHandler::IsKeyPressed(KeyCode::Q) || InputHandler::IsKeyPressed(KeyCode::E))
	{
		float rollInputAxis = InputHandler::GetInputAxis1D(KeyCode::Q, KeyCode::E);
		glm::vec3 rotation(0.0f);
		rotation.z = rollInputAxis;

		m_Transform.Rotate(rotation);
		rotated = true;
	}

	if (translated || rotated)
	{
		m_ViewMatrix = glm::inverse(m_Transform.GetTransformMatrix());
		m_ViewProjectionMatrix = m_ProjectionMatrix * m_ViewMatrix;

		MakeViewFrustumPlanes();
	}

	if (m_DebugDrawViewFrustum)
		DebugDrawViewFrustum();
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
	const glm::vec3& cameraPosition = m_Transform.GetPosition();
	const Transform& invTransform = m_Transform.GetInverse();
	const glm::vec3& cameraRight = invTransform.Right();
	const glm::vec3& cameraUp = invTransform.Up();
	const glm::vec3& cameraForward = invTransform.Forward();
	
	const glm::vec3& nearCenter = cameraPosition + cameraForward * m_ViewFrustum.Near;
	const glm::vec3& farCenter = cameraPosition + cameraForward * m_ViewFrustum.Far;

	// Near / far
	m_ViewFrustum.Planes[4] = { nearCenter, cameraForward };
	m_ViewFrustum.Planes[5] = { farCenter, -cameraForward };

	glm::vec3 aux = glm::vec3(0.0f);
	glm::vec3 normal = glm::vec3(0.0f);

	// Top
	aux = glm::normalize((nearCenter + cameraUp * m_ViewFrustum.NearHeight) - cameraPosition);
	normal = glm::cross(cameraRight, aux);
	m_ViewFrustum.Planes[0] = { nearCenter + cameraUp * m_ViewFrustum.NearHeight, normal };

	// Bottom
	aux = glm::normalize((nearCenter - cameraUp * m_ViewFrustum.NearHeight) - cameraPosition);
	normal = glm::cross(aux, cameraRight);
	m_ViewFrustum.Planes[1] = { nearCenter - cameraUp * m_ViewFrustum.NearHeight, normal };

	// Left
	aux = glm::normalize((nearCenter - cameraRight * m_ViewFrustum.NearWidth) - cameraPosition);
	normal = glm::cross(cameraUp, aux);
	m_ViewFrustum.Planes[2] = { nearCenter - cameraRight * m_ViewFrustum.NearWidth, normal };

	// Right
	aux = glm::normalize((nearCenter + cameraRight * m_ViewFrustum.NearWidth) - cameraPosition);
	normal = glm::cross(aux, cameraUp);
	m_ViewFrustum.Planes[3] = { nearCenter + cameraRight * m_ViewFrustum.NearWidth, normal };
}

void Camera::DebugDrawViewFrustum()
{
	auto renderer = Application::Get().GetRenderer();
	float debugOffset = 0.5f;

	const glm::vec3& cameraPosition = m_Transform.GetPosition();
	const Transform& invTransform = m_Transform.GetInverse();
	const glm::vec3& cameraRight = invTransform.Right();
	const glm::vec3& cameraUp = invTransform.Up();
	const glm::vec3& cameraForward = invTransform.Forward();

	const ViewFrustum::Plane& nearPlane = m_ViewFrustum.Planes[4];
	const ViewFrustum::Plane& farPlane = m_ViewFrustum.Planes[5];

	float nearPlaneHalfWidth = m_ViewFrustum.NearWidth / 2;
	float nearPlaneHalfHeight = m_ViewFrustum.NearHeight / 2;

	glm::vec3 topLeftNearPlane = nearPlane.Point - cameraRight * nearPlaneHalfWidth + cameraUp * nearPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 topRightNearPlane = nearPlane.Point + cameraRight * nearPlaneHalfWidth + cameraUp * nearPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 bottomRightNearPlane = nearPlane.Point + cameraRight * nearPlaneHalfWidth - cameraUp * nearPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 bottomLeftNearPlane = nearPlane.Point - cameraRight * nearPlaneHalfWidth - cameraUp * nearPlaneHalfHeight + cameraForward * debugOffset;

	renderer->Submit(topLeftNearPlane, topRightNearPlane, glm::vec4(0.8f, 0.8f, 0.0f, 1.0f));
	renderer->Submit(topRightNearPlane, bottomRightNearPlane, glm::vec4(0.8f, 0.8f, 0.0f, 1.0f));
	renderer->Submit(bottomRightNearPlane, bottomLeftNearPlane, glm::vec4(0.8f, 0.8f, 0.0f, 1.0f));
	renderer->Submit(bottomLeftNearPlane, topLeftNearPlane, glm::vec4(0.8f, 0.8f, 0.0f, 1.0f));

	float farPlaneHalfWidth = m_ViewFrustum.FarWidth / 2;
	float farPlaneHalfHeight = m_ViewFrustum.FarHeight / 2;

	glm::vec3 topLeftFarPlane = farPlane.Point - cameraRight * farPlaneHalfWidth + cameraUp * farPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 topRightFarPlane = farPlane.Point + cameraRight * farPlaneHalfWidth + cameraUp * farPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 bottomRightFarPlane = farPlane.Point + cameraRight * farPlaneHalfWidth - cameraUp * farPlaneHalfHeight + cameraForward * debugOffset;
	glm::vec3 bottomLeftFarPlane = farPlane.Point - cameraRight * farPlaneHalfWidth - cameraUp * farPlaneHalfHeight + cameraForward * debugOffset;

	renderer->Submit(topLeftFarPlane, topRightFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(topRightFarPlane, bottomRightFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(bottomRightFarPlane, bottomLeftFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(bottomLeftFarPlane, topLeftFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	renderer->Submit(topLeftNearPlane, topLeftFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(bottomLeftFarPlane, bottomLeftNearPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	renderer->Submit(topLeftNearPlane, topLeftFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(topRightFarPlane, topRightNearPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	renderer->Submit(topRightNearPlane, topRightFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(bottomRightFarPlane, bottomRightNearPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));

	renderer->Submit(bottomLeftNearPlane, bottomLeftFarPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
	renderer->Submit(bottomRightFarPlane, bottomRightNearPlane, glm::vec4(0.8f, 0.0f, 0.8f, 1.0f));
}
