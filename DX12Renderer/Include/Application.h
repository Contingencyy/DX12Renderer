#pragma once

class Window;
class Renderer;
class GUI;
class Scene;
class InputHandler;

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
	float GetLastFrameTime() const { return m_LastFrameTime; }

	Window* GetWindow() const { return m_Window.get(); }
	Renderer* GetRenderer() const { return m_Renderer.get(); }
	InputHandler* GetInputHandler() const { return m_InputHandler.get(); }

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
	std::unique_ptr<Renderer> m_Renderer = nullptr;
	std::unique_ptr<GUI> m_GUI = nullptr;
	std::unique_ptr<InputHandler> m_InputHandler = nullptr;
	std::unique_ptr<Scene> m_Scene = nullptr;

	float m_LastFrameTime = 0.0f;

	bool m_Initialized = false;

};
