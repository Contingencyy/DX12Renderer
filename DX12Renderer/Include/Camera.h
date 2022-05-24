#pragma once

class Camera
{
public:
	Camera();
	Camera(const glm::vec3& pos, const glm::vec3& target, float fov, float width, float height, float near = 0.1f, float far = 1000.0f);
	~Camera();

	void ResizeProjection(float width, float height);

	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };

private:
	glm::mat4 m_ViewMatrix;
	glm::mat4 m_ProjectionMatrix;
	glm::mat4 m_ViewProjectionMatrix;

	float m_FOV = 60.0f;
	float m_Near = 0.1f;
	float m_Far = 1000.0f;

};
