#include "Pch.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/Texture.h"

#include <imgui/imgui.h>

SpotLightComponent::SpotLightComponent(const SpotLightData& spotLightData, const glm::vec3& position)
	: m_SpotLightData(spotLightData)
{
	m_SpotLightData.Position = position;
	m_SpotLightData.Range = MathHelper::SolveQuadraticFunc(m_SpotLightData.Attenuation.z, m_SpotLightData.Attenuation.y, m_SpotLightData.Attenuation.x - LIGHT_RANGE_EPSILON);

	glm::mat4 lightView = glm::lookAtLH(m_SpotLightData.Position, m_SpotLightData.Position + m_SpotLightData.Direction, glm::vec3(0.0f, 0.0f, 1.0f));
	// This will construct a camera with a reverse-z perspective projection and an infinite far plane
	m_Camera = Camera(lightView, glm::degrees(m_SpotLightData.OuterConeAngle), 1.0f, m_SpotLightData.Range, 0.1f);

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();
	m_ShadowMap = std::make_shared<Texture>("Spotlight shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.ShadowMapResolution.x, renderSettings.ShadowMapResolution.y));

	m_SpotLightData.ViewProjection = m_Camera.GetViewProjection();
	m_SpotLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorHeapIndex(DescriptorType::SRV);

	m_GUIData.Direction = m_SpotLightData.Direction;
	m_GUIData.InnerConeAngle = glm::degrees(m_SpotLightData.InnerConeAngle);
	m_GUIData.OuterConeAngle = glm::degrees(m_SpotLightData.OuterConeAngle);
}

SpotLightComponent::~SpotLightComponent()
{
}

void SpotLightComponent::Update(float deltaTime)
{
}

void SpotLightComponent::Render(const Transform& transform)
{
	Renderer::Submit(m_SpotLightData, m_Camera, m_ShadowMap);
}

void SpotLightComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Spot Light"))
	{
		if (ImGui::DragFloat3("Position", glm::value_ptr(m_SpotLightData.Position), 0.1f))
		{
			glm::mat4 lightView = glm::lookAtLH(m_SpotLightData.Position, m_SpotLightData.Position + m_SpotLightData.Direction, glm::vec3(0.0f, 0.0f, 1.0f));
			m_Camera.SetViewMatrix(lightView);
			m_SpotLightData.ViewProjection = m_Camera.GetViewProjection();
		}
		if (ImGui::DragFloat3("Direction", glm::value_ptr(m_GUIData.Direction), 0.001f, -1000.0f, 1000.0f))
		{
			m_SpotLightData.Direction = glm::normalize(m_GUIData.Direction);
			glm::mat4 lightView = glm::lookAtLH(m_SpotLightData.Position, m_SpotLightData.Position + m_SpotLightData.Direction, glm::vec3(0.0f, 0.0f, 1.0f));
			m_Camera.SetViewMatrix(lightView);
			m_SpotLightData.ViewProjection = m_Camera.GetViewProjection();
		}
		ImGui::Text("Range (from attenuation): %.3f", m_SpotLightData.Range);
		if (ImGui::DragFloat3("Attenuation", glm::value_ptr(m_SpotLightData.Attenuation), 0.00001f, 0.0f, 100.0f, "%.7f"))
			m_SpotLightData.Range = MathHelper::SolveQuadraticFunc(m_SpotLightData.Attenuation.z, m_SpotLightData.Attenuation.y, m_SpotLightData.Attenuation.x - LIGHT_RANGE_EPSILON);
		if (ImGui::DragFloat("Inner cone angle", &m_GUIData.InnerConeAngle, 0.1f, 180.0f))
			m_SpotLightData.InnerConeAngle = glm::radians(m_GUIData.InnerConeAngle);
		if (ImGui::DragFloat("Outer cone angle", &m_GUIData.OuterConeAngle, 0.1f, 180.0f))
			m_SpotLightData.OuterConeAngle = glm::radians(m_GUIData.OuterConeAngle);
		ImGui::DragFloat3("Ambient", glm::value_ptr(m_SpotLightData.Ambient), 0.01f, 1000.0f);
		ImGui::DragFloat3("Diffuse", glm::value_ptr(m_SpotLightData.Diffuse), 0.01f, 1000.0f);
	}
}
