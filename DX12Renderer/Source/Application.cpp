#include "Pch.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/Renderer.h"
#include "Graphics/DebugRenderer.h"
#include "Graphics/Backend/RenderBackend.h"
#include "GUI.h"
#include "Scene/Scene.h"
#include "InputHandler.h"
#include "Resource/ResourceManager.h"

#include <imgui/imgui.h>

static Application* s_Instance = nullptr;

void Application::Create()
{
	if (!s_Instance)
	{
		s_Instance = new Application();
	}
}

Application& Application::Get()
{
	return *s_Instance;
}

void Application::Destroy()
{
	delete s_Instance;
	s_Instance = nullptr;
}

Application::Application()
{
}

Application::~Application()
{
}

void Application::Initialize(HINSTANCE hInst, uint32_t width, uint32_t height)
{
	Random::Initialize();

	WindowProps windowProps = {};
	windowProps.Title = L"DX12 Renderer";
	windowProps.Width = width;
	windowProps.Height = height;

	m_Window = std::make_unique<Window>();
	m_Window->Initialize(windowProps);
	m_Window->Show();
	LOG_INFO("[Window] Initialized Window");

	Renderer::Initialize(m_Window->GetHandle(), m_Window->GetWidth(), m_Window->GetHeight());
	LOG_INFO("[Renderer] Initialized Renderer");

	DebugRenderer::Initialize(m_Window->GetWidth(), m_Window->GetHeight());
	LOG_INFO("[DebugRenderer] Initialized DebugRenderer");

	m_GUI = std::make_unique<GUI>();
	m_GUI->Initialize(m_Window->GetHandle());
	LOG_INFO("[GUI] Initialized GUI");

	m_ResourceManager = std::make_unique<ResourceManager>();
	m_ResourceManager->LoadTexture("Resources/Textures/kermit.jpg", "Kermit");
	m_ResourceManager->LoadModel("Resources/Models/DamagedHelmet/DamagedHelmet.gltf", "DamagedHelmet");
	m_ResourceManager->LoadModel("Resources/Models/SponzaOld/Sponza.gltf", "SponzaOld");
	//m_ResourceManager->LoadModel("Resources/Models/SponzaPBR/NewSponza_Main_glTF_002.gltf", "SponzaPBR");

	m_Scene = std::make_unique<Scene>();
	m_Initialized = true;
}

void Application::Run()
{
	std::chrono::time_point current = std::chrono::high_resolution_clock::now(), last = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> deltaTime = std::chrono::duration<float>(0.0f);

	while (!m_Window->ShouldClose())
	{
		SCOPED_TIMER("Frametime");

		current = std::chrono::high_resolution_clock::now();
		deltaTime = current - last;

		PollEvents();
		Update(deltaTime.count());
		Render();

		last = current;
	}
}

void Application::Finalize()
{
	m_GUI->Finalize();
	LOG_INFO("Finalized GUI");

	DebugRenderer::Finalize();
	LOG_INFO("Finalized DebugRenderer");

	Renderer::Finalize();
	LOG_INFO("Finalized Renderer");

	m_Window->Finalize();
	LOG_INFO("Finalized Window");
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	if (width > 0 && height > 0)
	{
		Renderer::Resize(width, height);
		m_Scene->GetActiveCamera().ResizeProjection(static_cast<float>(width), static_cast<float>(height));
	}
}

void Application::PollEvents()
{
	m_Window->PollEvents();
}

void Application::Update(float deltaTime)
{
	SCOPED_TIMER("Application::Update");
	m_GUI->Update(deltaTime);
	m_Scene->Update(deltaTime);
}

void Application::Render()
{
	SCOPED_TIMER("Application::Render");

	Renderer::BeginFrame();

	const Camera& sceneCamera = m_Scene->GetActiveCamera();
	Renderer::BeginScene(sceneCamera);
	DebugRenderer::BeginScene(sceneCamera);

	m_Scene->Render();
	Renderer::Render();
	DebugRenderer::Render();

	if (m_RenderGUI)
	{
		m_GUI->BeginFrame();
		m_Scene->OnImGuiRender();

		ImGui::Begin("Rendering");
		Renderer::OnImGuiRender();
		ImGui::Separator();
		RenderBackend::OnImGuiRender();
		ImGui::Separator();
		DebugRenderer::OnImGuiRender();
		ImGui::End();

		Profiler::OnImGuiRender();

		m_GUI->EndFrame();
	}

	Profiler::Reset();

	DebugRenderer::EndScene();
	Renderer::EndScene();

	Renderer::EndFrame();
}
