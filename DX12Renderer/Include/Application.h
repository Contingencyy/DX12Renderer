#pragma once

class Window;
class Renderer;

class Application
{
public:
	static Application& Get();

	void Initialize(HINSTANCE hInst, uint32_t width, uint32_t height);
	void Run();
	void Finalize();

	bool IsInitialized() const { return m_Initialized; }

	Window* GetWindow() const { return m_Window; }
	Renderer* GetRenderer() const { return m_Renderer; }

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

	bool m_Initialized = false;

};
