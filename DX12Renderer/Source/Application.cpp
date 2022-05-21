#include "Pch.h"
#include "Application.h"
#include "Window.h"
#include "Renderer.h"
#include "GUI.h"

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
	m_Window = new Window;
	m_Window->Initialize(hInst, width, height);
	m_Window->Show();

	Logger::Log("Initialized Window", Logger::Severity::INFO);

	m_Renderer = new Renderer;
	m_Renderer->Initialize(m_Window->GetHandle(), width, height);

	Logger::Log("Initialized Renderer", Logger::Severity::INFO);

	m_GUI = new GUI;
	m_GUI->Initialize(m_Window->GetHandle());

	Logger::Log("Initialized GUI", Logger::Severity::INFO);

	m_Initialized = true;
}

void Application::Run()
{
	std::chrono::time_point current = std::chrono::high_resolution_clock::now(), last = std::chrono::high_resolution_clock::now();
	std::chrono::duration<float> deltaTime = std::chrono::duration<float>(0.0f);

	while (!m_Window->ShouldClose())
	{
		current = std::chrono::high_resolution_clock::now();
		deltaTime = current - last;

		PollEvents();
		Update(deltaTime.count());
		Render();
		
		last = current;
		m_LastFrameTime = deltaTime;
	}
}

void Application::Finalize()
{
	m_GUI->Finalize();
	Logger::Log("Finalized GUI", Logger::Severity::INFO);

	m_Renderer->Finalize();
	Logger::Log("Finalized Renderer", Logger::Severity::INFO);

	m_Window->Finalize();
	Logger::Log("Finalized Window", Logger::Severity::INFO);

	delete m_GUI;
	delete m_Renderer;
	delete m_Window;
}

void Application::PollEvents()
{
	m_Window->PollEvents();
}

void Application::Update(float deltaTime)
{
}

void Application::Render()
{
	m_Renderer->BeginFrame();

	m_Renderer->Render();
	m_GUI->Render();

	m_Renderer->EndFrame();
}
