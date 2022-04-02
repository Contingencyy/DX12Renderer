#include "Pch.h"
#include "Application.h"
#include "Window.h"
#include "Renderer.h"

Application& Application::Get()
{
	static Application s_Instance;
	return s_Instance;
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

	m_Renderer = new Renderer;
	m_Renderer->Initialize(m_Window->GetHandle(), width, height);

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
		m_Renderer->Render();

		last = current;
	}
}

void Application::Finalize()
{
	m_Renderer->Finalize();
	m_Window->Finalize();

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
