#include "Pch.h"
#include "Components/PointLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

PointLightComponent::PointLightComponent(const PointLightData& pointLightData, const glm::vec3& position)
	: m_PointLightData(pointLightData)
{
	m_PointLightData.Position = position;
	m_PointLightData.Range = MathHelper::SolveQuadraticFunc(m_PointLightData.Attenuation.z, m_PointLightData.Attenuation.y, m_PointLightData.Attenuation.x - LIGHT_RANGE_EPSILON);

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

	m_ShadowMap = std::make_shared<Texture>("Pointlight shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		renderSettings.ShadowMapSize, renderSettings.ShadowMapSize));
	m_PointLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorIndex(DescriptorType::SRV);
}

PointLightComponent::~PointLightComponent()
{
}

void PointLightComponent::Update(float deltaTime)
{
}

void PointLightComponent::Render(const Camera& camera, const Transform& transform)
{
	Renderer::Submit(m_PointLightData, m_ShadowMap);
}

void PointLightComponent::OnImGuiRender()
{
}
