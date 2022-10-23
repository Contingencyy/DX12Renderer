#include "Pch.h"
#include "Scene/LightObject.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

/*

	Since distance attenuation is a quadratic function, we can calculate a light's range based on its constant, linear, and quadratic falloff.
	The light range epsilon defines the attenuation value that is required before the light source will be ignored for further calculations in the shaders.
	We calculate it once on the CPU so that the GPU does not have to do it for every fragment/pixel. Only applies to point and spotlights.
	In the shader we need to rescale the attenuation value to account for this cutoff, otherwise the cutoff will be visible.

*/

constexpr float LIGHT_RANGE_EPSILON = 1.0f / 0.01f;

DirectionalLightObject::DirectionalLightObject(const DirectionalLightData& dirLightData, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_DirectionalLightData(dirLightData)
{
	m_FrustumCullable = false;

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

	glm::mat4 lightView = glm::lookAtLH(glm::vec3(-m_DirectionalLightData.Direction.x, -m_DirectionalLightData.Direction.y, -m_DirectionalLightData.Direction.z) * 2000.0f, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	// Shadow map projection size should be calculated by the maximum scene bounds in terms of shadow casters and receivers
	float orthoSize = 2000.0f;
	// We use a reverse projection matrix here
	glm::mat4 lightProj = glm::orthoLH_ZO(-orthoSize, orthoSize, orthoSize, -orthoSize, 2250.0f, 0.1f);

	m_DirectionalLightData.ViewProjection = lightProj * lightView;

	m_ShadowMap = std::make_shared<Texture>("Directional light shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		renderSettings.ShadowMapSize, renderSettings.ShadowMapSize));
	m_DirectionalLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorIndex(DescriptorType::SRV);
}

DirectionalLightObject::~DirectionalLightObject()
{
}

void DirectionalLightObject::Update(float deltaTime)
{
}

void DirectionalLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_DirectionalLightData, m_ShadowMap);
}

PointLightObject::PointLightObject(const PointLightData& pointLightData, const std::string& name,
	const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_PointLightData(pointLightData)
{
	m_FrustumCullable = false;
	m_PointLightData.Position = m_Transform.GetPosition();
	m_PointLightData.Range = MathHelper::SolveQuadraticFunc(m_PointLightData.Attenuation.z, m_PointLightData.Attenuation.y, m_PointLightData.Attenuation.x - LIGHT_RANGE_EPSILON);

	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();



	m_ShadowMap = std::make_shared<Texture>("Pointlight shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		renderSettings.ShadowMapSize, renderSettings.ShadowMapSize));
	m_PointLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorIndex(DescriptorType::SRV);
}

PointLightObject::~PointLightObject()
{
}

void PointLightObject::Update(float deltaTime)
{
	m_PointLightData.Position = m_Transform.GetPosition();
}

void PointLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_PointLightData, m_ShadowMap);
}

SpotLightObject::SpotLightObject(const SpotLightData& spotLightData, const std::string& name, const glm::vec3& translation, const glm::vec3& rotation, const glm::vec3& scale)
	: SceneObject(name, translation, rotation, scale), m_SpotLightData(spotLightData)
{
	m_FrustumCullable = false;
	m_SpotLightData.Position = m_Transform.GetPosition();
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

SpotLightObject::~SpotLightObject()
{
}

void SpotLightObject::Update(float deltaTime)
{
	m_SpotLightData.Position = m_Transform.GetPosition();
}

void SpotLightObject::Render(const Camera& camera)
{
	Renderer::Submit(m_SpotLightData, m_ShadowMap);
}
