#include "Pch.h"
#include "Graphics/ParticleSystem.h"
#include "Graphics/Buffer.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"

struct Vertex
{
	glm::vec3 Position;
	//glm::vec2 TexCoord;
};

//std::vector<Vertex> QuadVertices = {
//    { glm::vec3(-0.5f, -0.5f, 0.0f), glm::vec2(0.0f, 1.0f) },
//    { glm::vec3(-0.5f, 0.5f, 0.0f), glm::vec2(0.0f, 0.0f) },
//    { glm::vec3(0.5f, 0.5f, 0.0f), glm::vec2(1.0f, 0.0f) },
//    { glm::vec3(0.5f, -0.5f, 0.0f), glm::vec2(1.0f, 1.0f) }
//};

std::vector<Vertex> QuadVertices = {
	{ glm::vec3(-0.5f, -0.5f, 0.0f) },
	{ glm::vec3(-0.5f, 0.5f, 0.0f) },
	{ glm::vec3(0.5f, 0.5f, 0.0f) },
	{ glm::vec3(0.5f, -0.5f, 0.0f) }
};

std::vector<WORD> QuadIndices = {
	0, 1, 2,
	2, 3, 0
};

ParticleSystem::ParticleSystem()
{
	m_ParticlePool.resize(1000);
	m_ParticleInstanceData.reserve(1000);

	m_QuadVB = std::make_shared<Buffer>(BufferDesc(), QuadVertices.size(), sizeof(QuadVertices[0]));
	Buffer quadVBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadVB->GetAlignedSize());
	Application::Get().GetRenderer()->CopyBuffer(quadVBUploadBuffer, *m_QuadVB, &QuadVertices[0]);

	m_QuadIB = std::make_shared<Buffer>(BufferDesc(), QuadIndices.size(), sizeof(QuadIndices[0]));
	Buffer quadIBUploadBuffer(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadIB->GetAlignedSize());
	Application::Get().GetRenderer()->CopyBuffer(quadIBUploadBuffer, *m_QuadIB, &QuadIndices[0]);

	m_QuadInstanceDataBuffer = std::make_shared<Buffer>(BufferDesc(), m_ParticlePool.size(), sizeof(ParticleInstanceData));
	m_UploadBuffer = std::make_shared<Buffer>(BufferDesc(D3D12_HEAP_TYPE_UPLOAD, D3D12_RESOURCE_STATE_GENERIC_READ), m_QuadInstanceDataBuffer->GetAlignedSize());
}

void ParticleSystem::Update(float deltaTime)
{
	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		if (particle.LifeRemaining <= 0.0f)
		{
			particle.Active = false;
			continue;
		}

		particle.LifeRemaining -= deltaTime;
		particle.Position += particle.Velocity * deltaTime;
		particle.Rotation += particle.BaseRotation * deltaTime;
	}
}

void ParticleSystem::Render()
{
	m_ParticleInstanceData.clear();

	for (auto& particle : m_ParticlePool)
	{
		if (!particle.Active)
			continue;

		// Lerp particle values based on life remaining
		float life = particle.LifeRemaining / particle.LifeTime;
		glm::vec4 color = glm::lerp(particle.ColorEnd, particle.ColorBegin, life);
		color.a = color.a * life;

		float size = glm::lerp(particle.SizeEnd, particle.SizeBegin, life);

		// Set particle instance data
		glm::mat4 transform = glm::translate(glm::identity<glm::mat4>(), { particle.Position.x, particle.Position.y, 0.0f })
			* glm::rotate(glm::identity<glm::mat4>(), particle.Rotation, { 0.0f, 0.0f, 1.0f })
			* glm::scale(glm::identity<glm::mat4>(), { size, size, 1.0f });
		m_ParticleInstanceData.emplace_back(transform, color);
	}

	// Render
	auto renderer = Application::Get().GetRenderer();
	auto commandList = renderer->m_CommandQueueDirect->GetCommandList();

	if (m_ParticleInstanceData.size() > 0)
		commandList->CopyBuffer(*m_UploadBuffer, *m_QuadInstanceDataBuffer, &m_ParticleInstanceData[0]);

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(renderer->m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
		renderer->m_CurrentBackBufferIndex, renderer->m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(renderer->m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());

	commandList->SetViewports(1, &renderer->m_Viewport);
	commandList->SetScissorRects(1, &renderer->m_ScissorRect);
	commandList->SetRenderTargets(1, &rtv, &dsv);

	commandList->SetPipelineState(renderer->m_d3d12PipelineState.Get());
	commandList->SetRootSignature(renderer->m_d3d12RootSignature.Get());
	commandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	glm::mat4 projection = glm::perspectiveFovLH_ZO(glm::radians(60.0f), static_cast<float>(renderer->m_RenderSettings.Resolution.x),
		static_cast<float>(renderer->m_RenderSettings.Resolution.y), 0.1f, 1000.0f);
	glm::mat4 view = glm::lookAtLH(glm::vec3(0.0f, 0.0f, -10.0f), glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
	glm::mat4 viewProjection = projection * view;
	commandList->SetRoot32BitConstants(0, sizeof(glm::mat4) / 4, &viewProjection, 0);

	commandList->SetVertexBuffers(0, 1, *m_QuadVB);
	commandList->SetVertexBuffers(1, 1, *m_QuadInstanceDataBuffer);
	commandList->SetIndexBuffer(*m_QuadIB);

	commandList->DrawIndexed(QuadIndices.size(), m_ParticleInstanceData.size());

	uint64_t fenceValue = renderer->m_CommandQueueDirect->ExecuteCommandList(commandList);
	renderer->m_CommandQueueDirect->WaitForFenceValue(fenceValue);

	m_NumActiveParticles = m_ParticleInstanceData.size();
}

void ParticleSystem::Emit(const ParticleProps& particleProps)
{
	Particle& particle = m_ParticlePool[m_PoolIndex];
	particle.Active = true;
	particle.Position = particleProps.Position;
	particle.Rotation = Random::FloatRange(1.0f, 2.0f) * glm::pi<float>();

	particle.Velocity = particleProps.Velocity;
	particle.Velocity.x += particleProps.VelocityVariation.x * Random::FloatRange(-0.5f, 0.5f);
	particle.Velocity.y += particleProps.VelocityVariation.y * Random::FloatRange(-0.5f, 0.5f);

	particle.ColorBegin = particleProps.ColorBegin;
	particle.ColorEnd = particleProps.ColorEnd;

	particle.LifeTime = particleProps.LifeTime;
	particle.LifeRemaining = particleProps.LifeTime;
	particle.BaseRotation = particleProps.BaseRotation + particleProps.BaseRotationVariation * Random::FloatRange(-0.5f, 0.5f);
	particle.SizeBegin = particleProps.SizeBegin + particleProps.SizeVariation * Random::FloatRange(-0.5f, 0.5f);
	particle.SizeEnd = particleProps.SizeEnd;

	m_PoolIndex = --m_PoolIndex % m_ParticlePool.size();
}
