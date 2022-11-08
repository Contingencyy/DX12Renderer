#pragma once
#include "Transform.h"
#include "ViewFrustum.h"

class Camera
{
public:
	Camera() = default;
	// Constructs a perspective camera with a normal near and far plane and normal z
	Camera(const glm::vec3& pos, float fov, float width, float height, float near, float far);
	// Constructs a perspective camera with a reverse-z projection
	Camera(const glm::mat4& view, float fov, float aspect, float near, float far);
	// Constructs an orthographic camera with a reverse-z projection
	Camera(const glm::mat4& view, float left, float right, float bottom, float top, float near, float far);
	~Camera();

	void Update(float deltaTime);
	void ResizeProjection(float width, float height);
	void SetViewMatrix(const glm::mat4& view);
	void OnImGuiRender();

	const ViewFrustum& GetViewFrustum() const { return m_ViewFrustum; }
	float GetExposure() const { return m_Exposure; }
	float GetGamma() const { return m_Gamma; }
	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };
	bool IsFrustumCullingEnabled() const { return m_EnableFrustumCulling; }
	const Transform& GetTransform() const { return m_Transform; }

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
	float m_Width = 1280.0f, m_Height = 720.0f;
	float m_FOV = 60.0f, m_AspectRatio = 16.0f / 9.0f;
	float m_Yaw = 0.0f, m_Pitch = 0.0f;
	float m_Exposure = 1.5f;
	float m_Gamma = 2.2f;

	glm::vec2 m_RotationAnchorPoint = glm::vec2(0.0f);
	bool m_SetAnchorPointOnClick = true;

	ViewFrustum m_ViewFrustum;
	bool m_EnableFrustumCulling = true;

};
