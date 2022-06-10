#pragma once

class Camera
{
public:
	Camera();
	Camera(const glm::vec3& pos, float fov, float width, float height, float near = 0.1f, float far = 1000.0f);
	~Camera();

	void Update(float deltaTime);
	void ResizeProjection(float width, float height);

	glm::mat4 GetViewProjection() const { return m_ViewProjectionMatrix; };

private:
	glm::vec3 Forward() const;
	glm::vec3 Right() const;
	glm::vec3 Up() const;

private:
	glm::mat4 m_ViewMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ProjectionMatrix = glm::identity<glm::mat4>();
	glm::mat4 m_ViewProjectionMatrix = glm::identity<glm::mat4>();

	glm::vec3 m_Position = glm::vec3(0.0f);
	glm::vec3 m_Velocity = glm::vec3(0.0f);

	float m_Speed = 5.0f;

	float m_FOV = 60.0f;
	float m_Near = 0.1f;
	float m_Far = 1000.0f;

};
