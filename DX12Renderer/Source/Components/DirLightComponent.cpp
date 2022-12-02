#include "Pch.h"
#include "Components/DirLightComponent.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/Renderer.h"
#include "Graphics/Texture.h"

#include <imgui/imgui.h>

DirLightComponent::DirLightComponent(const DirectionalLightData& dirLightData)
	: m_DirectionalLightData(dirLightData)
{
	const Renderer::RenderSettings& renderSettings = Renderer::GetSettings();

	float orthoSize = static_cast<float>(renderSettings.ShadowMapResolution.x);
	glm::mat4 lightView = glm::lookAtLH(glm::vec3(-m_DirectionalLightData.Direction.x, -m_DirectionalLightData.Direction.y, -m_DirectionalLightData.Direction.z) * orthoSize, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	// Shadow map projection size should be calculated by the maximum scene bounds in terms of shadow casters and receivers
	// We use a reverse projection matrix here
	m_Camera = Camera(lightView, -orthoSize, orthoSize, orthoSize, -orthoSize, 2250.0f, 0.1f);

	m_ShadowMap = std::make_shared<Texture>("Directional light shadow map", TextureDesc(TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ, TextureFormat::TEXTURE_FORMAT_DEPTH32,
		TextureDimension::TEXTURE_DIMENSION_2D, renderSettings.ShadowMapResolution.x, renderSettings.ShadowMapResolution.y));

	m_DirectionalLightData.ShadowMapIndex = m_ShadowMap->GetDescriptorHeapIndex(DescriptorType::SRV);
	m_DirectionalLightData.ViewProjection = m_Camera.GetViewProjection();

	m_GUIData.Direction = m_DirectionalLightData.Direction;
}

DirLightComponent::~DirLightComponent()
{
}

void DirLightComponent::Update(float deltaTime)
{
}

void DirLightComponent::Render()
{
	Renderer::Submit(m_DirectionalLightData, m_Camera, m_ShadowMap);
}

void DirLightComponent::OnImGuiRender()
{
	if (ImGui::CollapsingHeader("Directional Light"))
	{
		if (ImGui::DragFloat3("Direction", glm::value_ptr(m_GUIData.Direction), 0.001f, -1000.0f, 1000.0f))
		{
			m_DirectionalLightData.Direction = glm::normalize(m_GUIData.Direction);
			float orthoSize = 2000.0f;

			glm::mat4 lightView = glm::lookAtLH(glm::vec3(-m_DirectionalLightData.Direction.x, -m_DirectionalLightData.Direction.y, -m_DirectionalLightData.Direction.z) * orthoSize, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			m_Camera.SetViewMatrix(lightView);

			m_DirectionalLightData.ViewProjection = m_Camera.GetViewProjection();
		}
		ImGui::DragFloat3("Ambient", glm::value_ptr(m_DirectionalLightData.Ambient), 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat3("Diffuse", glm::value_ptr(m_DirectionalLightData.Diffuse), 0.01f, 0.0f, 1000.0f);
	}
}
