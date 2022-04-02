#pragma once

class Window;

class Application
{
public:
	static Application& Get();

	void Initialize(HINSTANCE hInst, uint32_t width, uint32_t height);
	void Run();
	void Finalize();

	bool IsInitialized() const { return m_Initialized; }

	std::shared_ptr<Window> GetWindow() const { return m_Window; }

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

private:
	std::shared_ptr<Window> m_Window = nullptr;

	bool m_Initialized = false;

};
