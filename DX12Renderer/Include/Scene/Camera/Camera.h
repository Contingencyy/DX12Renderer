#pragma once
#include "Transform.h"
#include "ViewFrustum.h"

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& pos, float fov, float width, float height, float near = 0.1f, float far = 10000.0f);
	~Camera();

	void Update(float deltaTime);
	void ResizeProjection(float width, float height);

	const ViewFrustum& GetViewFrustum() const { return m_ViewFrustum; }
	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };
	bool IsFrustumCullingEnabled() const { return m_EnableFrustumCulling; }

private:
	bool UpdateMovement(float deltaTime);
	bool UpdateRotation(float deltaTime);

private:
	Transform m_Transform;

	glm::mat4 m_ViewMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();

	float m_Speed = 250.0f;
	float m_RotationSpeed = 0.5f, m_RotationDeadZone = 25.0f;
	float m_FOV = 60.0f, m_AspectRatio = 16.0f / 9.0f;
	float m_Yaw = 0.0f, m_Pitch = 0.0f;

	ViewFrustum m_ViewFrustum;
	bool m_EnableFrustumCulling = true;

};
