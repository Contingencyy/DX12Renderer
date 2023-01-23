#include "Pch.h"
#include "Components/DirLightComponent.h"
#include "Scene/Camera/Camera.h"
#include "Graphics/Renderer.h"
#include "Graphics/RenderAPI.h"
#include "Graphics/Texture.h"

#include <imgui/imgui.h>

DirLightComponent::DirLightComponent(const DirectionalLightData& dirLightData)
	: m_DirectionalLightData(dirLightData)
{
	const Resolution& shadowRes = Renderer::GetShadowResolution();

	float orthoSize = static_cast<float>(shadowRes.x);
	glm::mat4 lightView = glm::lookAtLH(glm::vec3(-m_DirectionalLightData.Direction.x, -m_DirectionalLightData.Direction.y, -m_DirectionalLightData.Direction.z) * orthoSize, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
	// Shadow map projection size should be calculated by the maximum scene bounds in terms of shadow casters and receivers
	// We use a reverse projection matrix here
	m_Camera = Camera(lightView, -orthoSize, orthoSize, orthoSize, -orthoSize, 2250.0f, 0.1f);

	// Create shadow map
	TextureDesc shadowMapDesc = {};
	shadowMapDesc.Usage = TextureUsage::TEXTURE_USAGE_DEPTH | TextureUsage::TEXTURE_USAGE_READ;
	shadowMapDesc.Format = TextureFormat::TEXTURE_FORMAT_DEPTH32;
	shadowMapDesc.Width = shadowRes.x;
	shadowMapDesc.Height = shadowRes.y;
	shadowMapDesc.DebugName = "Directional light shadow map";

	m_ShadowMap = Renderer::CreateTexture(shadowMapDesc);

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
	if (ImGui::CollapsingHeader("Directional Light Component"))
	{
		ImGui::Indent(20.0f);

		if (ImGui::DragFloat3("Direction", glm::value_ptr(m_GUIData.Direction), 0.001f, -1000.0f, 1000.0f))
		{
			m_DirectionalLightData.Direction = glm::normalize(m_GUIData.Direction);
			float orthoSize = 2000.0f;

			glm::mat4 lightView = glm::lookAtLH(glm::vec3(-m_DirectionalLightData.Direction.x, -m_DirectionalLightData.Direction.y, -m_DirectionalLightData.Direction.z) * orthoSize, glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f));
			m_Camera.SetViewMatrix(lightView);

			m_DirectionalLightData.ViewProjection = m_Camera.GetViewProjection();
		}
		ImGui::DragFloat3("Ambient", glm::value_ptr(m_DirectionalLightData.Ambient), 0.01f, 0.0f, 1000.0f);
		ImGui::DragFloat3("Color", glm::value_ptr(m_DirectionalLightData.Color), 0.01f, 0.0f, 1000.0f);

		ImGui::Unindent(20.0f);
	}
}
