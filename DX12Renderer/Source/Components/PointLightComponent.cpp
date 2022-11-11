#include "Pch.h"
#include "Components/PointLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

#include <imgui/imgui.h>

const glm::vec3 LightViewDirections[6] = {
	{ 1.0f, 0.0f, 0.0f }, { -1.0f, 0.0f, 0.0f },
	{ 0.0f, 1.0f, 0.0f }, { 0.0f, -1.0f, 0.0 },
	{ 0.0f, 0.0f, 1.0f }, { 0.0f, 0.0f, -1.0f }
};

const glm::vec3 LightUpVectors[6] = {
	{ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f },
	{ 0.0f, 0.0f, -1.0f }, { 0.0f, 0.0f, 1.0f },
	{ 0.0f, 1.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }
};

PointLightComponent::PointLightComponent(const PointLightData& pointLightData, const glm::vec3& position)
	: m_PointLightData(pointLightData)
{
	m_PointLightData.Position = position;
	m_PointLightData.Range = MathHelper::SolveQuadraticFunc(m_PointLightData.Attenuation.z, m_PointLightData.Attenuation.y, m_PointLightData.Attenuation.x - LIGHT_RANGE_EPSILON);

	for (uint32_t i = 0; i < 6; ++i)
	{
		glm::mat4 lightViewFace = glm::lookAtLH(m_PointLightData.Position, m_PointLightData.Position + LightViewDirections[i], LightUpVectors[i]);
		// This will construct a camera with a reverse-z perspective projection and an infinite far plane
		m_Cameras[i] = Camera(lightViewFace, 90.0f, 1.0f, m_PointLightData.Range, 0.1f);
	}

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

	m_ShadowMap = std::make_shared<Texture>("Pointlight shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		TextureDimension::TEXTURE_DIMENSION_CUBE, renderSettings.ShadowMapResolution.x, renderSettings.ShadowMapResolution.y));
	m_PointLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorHeapIndex(DescriptorType::SRV);
}

PointLightComponent::~PointLightComponent()
{
}

void PointLightComponent::Update(float deltaTime)
{
}

void PointLightComponent::Render(const Transform& transform)
{
	Renderer::Submit(m_PointLightData, m_Cameras, m_ShadowMap);
}

void PointLightComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Point Light"))
	{
		if (ImGui::DragFloat3("Position", glm::value_ptr(m_PointLightData.Position), 0.1f))
		{
			for (uint32_t i = 0; i < 6; ++i)
			{
				glm::mat4 lightViewFace = glm::lookAtLH(m_PointLightData.Position, m_PointLightData.Position + LightViewDirections[i], LightUpVectors[i]);
				m_Cameras[i].SetViewMatrix(lightViewFace);
			}
		}
		ImGui::Text("Range (from attenuation): %.3f", m_PointLightData.Range);
		if (ImGui::DragFloat3("Attenuation", glm::value_ptr(m_PointLightData.Attenuation), 0.00001f, 0.0f, 100.0f, "%.7f"))
			m_PointLightData.Range = MathHelper::SolveQuadraticFunc(m_PointLightData.Attenuation.z, m_PointLightData.Attenuation.y, m_PointLightData.Attenuation.x - LIGHT_RANGE_EPSILON);
		ImGui::DragFloat3("Ambient", glm::value_ptr(m_PointLightData.Ambient), 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat3("Diffuse", glm::value_ptr(m_PointLightData.Diffuse), 0.01f, 0.0f, 1000.0f);
	}
}
