#include "Pch.h"
#include "Application.h"
#include "Window.h"
#include "Graphics/Renderer.h"
#include "GUI.h"
#include "Scene/Scene.h"
#include "InputHandler.h"
#include "ResourceManager.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

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
	LOG_INFO("Initialized Window");

	m_Renderer = std::make_unique<Renderer>();
	m_Renderer->Initialize(m_Window->GetWidth(), m_Window->GetHeight());
	LOG_INFO("Initialized Renderer");

	m_GUI = std::make_unique<GUI>();
	m_GUI->Initialize(m_Window->GetHandle());
	LOG_INFO("Initialized GUI");

	m_ResourceManager = std::make_unique<ResourceManager>();
	m_ResourceManager->LoadTexture("Resources/Textures/kermit.jpg", "Kermit");
	m_ResourceManager->LoadModel("Resources/Models/DamagedHelmet/DamagedHelmet.gltf", "DamagedHelmet");

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

	m_Renderer->Finalize();
	LOG_INFO("Finalized Renderer");

	m_Window->Finalize();
	LOG_INFO("Finalized Window");
}

void Application::OnWindowResize(uint32_t width, uint32_t height)
{
	if (width > 0 && height > 0)
	{
		m_Renderer->Resize(width, height);
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

	m_Renderer->BeginFrame(m_Scene->GetActiveCamera());
	m_GUI->BeginFrame();

	m_Scene->Render();
	m_Renderer->Render();

	m_Scene->ImGuiRender();
	m_Renderer->ImGuiRender();

	// Profiler
	{
		ImGui::Begin("Profiler");

		auto& timerResults = Profiler::Get().GetTimerResults();
		auto frameTimeIter = timerResults.find("Frametime");

		if (frameTimeIter != timerResults.end())
		{
			ImGui::Text("Frametime: %.3f ms", frameTimeIter->second.Duration);
			ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / (frameTimeIter->second.Duration)));
		}

		for (auto& timerResult : timerResults)
		{
			if (timerResult.first == "Frametime")
				continue;

			char buf[50];
			strcpy_s(buf, timerResult.second.Name);
			strcat_s(buf, ": %.3fms");

			ImGui::Text(buf, timerResult.second.Duration);
		}

		ImGui::End();
		Profiler::Get().Reset();
	}

	m_GUI->EndFrame();
	m_Renderer->EndFrame();
}
