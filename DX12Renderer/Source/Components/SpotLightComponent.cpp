#include "Pch.h"
#include "Components/SpotLightComponent.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

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
}
