#pragma once

class Window;
class Renderer;
class GUI;

class Application
{
public:
	static void Create();
	static Application& Get();
	static void Destroy();

	void Initialize(HINSTANCE hInst, uint32_t width, uint32_t height);
	void Run();
	void Finalize();

	bool IsInitialized() const { return m_Initialized; }
	std::chrono::duration<float> GetLastFrameTime() const { return m_LastFrameTime; }

	Window* GetWindow() const { return m_Window; }
	Renderer* GetRenderer() const { return m_Renderer; }
	GUI* GetGUI() const { return m_GUI; }

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
	Window* m_Window = nullptr;
	Renderer* m_Renderer = nullptr;
	GUI* m_GUI = nullptr;

	std::chrono::duration<float> m_LastFrameTime = std::chrono::duration<float>(0.0f);

	bool m_Initialized = false;

};
