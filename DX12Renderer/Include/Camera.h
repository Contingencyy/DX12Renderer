#pragma once
#include "Transform.h"

class Camera
{
public:
	Camera() = default;
	Camera(const glm::vec3& pos, float fov, float width, float height, float near = 0.1f, float far = 10000.0f);
	~Camera();

	void Update(float deltaTime);
	void ResizeProjection(float width, float height);

	bool IsPointInViewFrustum(const glm::vec3& point) const;
	bool IsSphereInViewFrustum(const glm::vec3& point, float radius) const;

	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };
	bool IsFrustumCullingEnabled() const { return m_EnableFrustumCulling; }

private:
	void UpdateViewFrustumBounds();
	void MakeViewFrustumPlanes();
	void DebugDrawViewFrustum();

private:
	Transform m_Transform;

	glm::mat4 m_ViewMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();

	float m_Speed = 250.0f;
	float m_RotationSpeed = 10.0f;
	float m_RotationDeadZone = 25.0f;
	float m_FOV = 60.0f;
	float m_AspectRatio = 16.0f / 9.0f;

	bool m_DebugDrawViewFrustum = false;
	bool m_EnableFrustumCulling = true;

	struct ViewFrustum
	{
		struct Plane
		{
			Plane() = default;
			Plane(const glm::vec3& p0, const glm::vec3& normal)
				 : Point(p0), Normal(normal) {}

			glm::vec3 Point = glm::vec3(0.0f);
			glm::vec3 Normal = glm::vec3(0.0f);
		};

		ViewFrustum() = default;

		Plane Planes[6];

		float Near = 0.1f;
		float NearWidth = 1280.0f;
		float NearHeight = 720.0f;

		float Far = 1000.0f;
		float FarWidth = 1280.0f;
		float FarHeight = 720.0f;

		float Tangent = 0.0f;
	};

	ViewFrustum m_ViewFrustum;

};
