#pragma once

class Window;
class Scene;

class Application
{
public:
	static void Create();
	static Application& Get();
	static void Destroy();

	void Initialize(HINSTANCE hInst, uint32_t width, uint32_t height);
	void Run();
	void Finalize();

	void OnWindowResize(uint32_t width, uint32_t height);

	bool IsInitialized() const { return m_Initialized; }
	void ToggleRenderGUI() { m_RenderGUI = !m_RenderGUI; }

	Window* GetWindow() const { return m_Window.get(); }

private:
	Application();

	Application(const Application& other) = delete;
	Application(Application&& other) = delete;

	Application& operator=(const Application& other) = delete;
	Application& operator=(Application&& other) = delete;

	~Application();

private:
	void PollEvents();
	void Update(float deltaTime);
	void Render();

private:
	std::unique_ptr<Window> m_Window = nullptr;
	std::unique_ptr<Scene> m_Scene = nullptr;

	bool m_Initialized = false;
	bool m_RenderGUI = true;

};
