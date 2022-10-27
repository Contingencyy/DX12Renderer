#include "Pch.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

#include <imgui/imgui.h>

SpotLightComponent::SpotLightComponent(const SpotLightData& spotLightData, const glm::vec3& position)
	: m_SpotLightData(spotLightData)
{
	m_SpotLightData.Position = position;
	m_SpotLightData.Range = MathHelper::SolveQuadraticFunc(m_SpotLightData.Attenuation.z, m_SpotLightData.Attenuation.y, m_SpotLightData.Attenuation.x - LIGHT_RANGE_EPSILON);

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

	glm::mat4 lightView = glm::lookAtLH(m_SpotLightData.Position, m_SpotLightData.Position + m_SpotLightData.Direction, glm::vec3(0.0f, 0.0f, 1.0f));
	// We use a reversed perspective projection with an infinite far plane to have more depth buffer precision because values between 0..0.5 have a lot more possible values than 0.5..1
	glm::mat4 lightProj = glm::perspectiveLH_ZO(m_SpotLightData.OuterConeAngle, 1.0f, m_SpotLightData.Range, 0.1f);
	lightProj[2][2] = 0.0f;
	lightProj[3][2] = 0.1f;
	m_SpotLightData.ViewProjection = lightProj * lightView;

	m_ShadowMap = std::make_shared<Texture>("Spotlight shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		renderSettings.ShadowMapSize, renderSettings.ShadowMapSize));
	m_SpotLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorIndex(DescriptorType::SRV);

	m_GUIData.Direction = m_SpotLightData.Direction;
	m_GUIData.InnerConeAngle = m_SpotLightData.InnerConeAngle;
	m_GUIData.OuterConeAngle = m_SpotLightData.OuterConeAngle;
}

SpotLightComponent::~SpotLightComponent()
{
}

void SpotLightComponent::Update(float deltaTime)
{
}

void SpotLightComponent::Render(const Camera& camera, const Transform& transform)
{
	Renderer::Submit(m_SpotLightData, m_ShadowMap);
}

void SpotLightComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Spot light"))
	{
		if (ImGui::DragFloat3("Direction", glm::value_ptr(m_GUIData.Direction), 0.001f, -1000.0f, 1000.0f))
		{
			m_SpotLightData.Direction = glm::normalize(m_GUIData.Direction);
			glm::mat4 lightView = glm::lookAtLH(m_SpotLightData.Position, m_SpotLightData.Position + m_SpotLightData.Direction, glm::vec3(0.0f, 0.0f, 1.0f));
			// We use a reversed perspective projection with an infinite far plane to have more depth buffer precision because values between 0..0.5 have a lot more possible values than 0.5..1
			glm::mat4 lightProj = glm::perspectiveLH_ZO(m_SpotLightData.OuterConeAngle, 1.0f, m_SpotLightData.Range, 0.1f);
			lightProj[2][2] = 0.0f;
			lightProj[3][2] = 0.1f;
			m_SpotLightData.ViewProjection = lightProj * lightView;
		}
		ImGui::DragFloat3("Attenuation", glm::value_ptr(m_SpotLightData.Attenuation), 0.00001f, 0.0f, 100.0f, "%.7f");
		if (ImGui::DragFloat("Inner cone angle", &m_GUIData.InnerConeAngle, 0.001f, 180.0f))
			m_SpotLightData.InnerConeAngle = glm::radians(m_GUIData.InnerConeAngle);
		if (ImGui::DragFloat("Outer cone angle", &m_GUIData.OuterConeAngle, 0.001f, 180.0f))
			m_SpotLightData.OuterConeAngle = glm::radians(m_GUIData.OuterConeAngle);
		ImGui::DragFloat3("Ambient", glm::value_ptr(m_SpotLightData.Ambient), 0.01f, 1000.0f);
		ImGui::DragFloat3("Diffuse", glm::value_ptr(m_SpotLightData.Diffuse), 0.01f, 1000.0f);
	}
}
