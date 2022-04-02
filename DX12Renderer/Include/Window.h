#pragma once

class Window
{
public:
	Window();
	~Window();

	void Initialize(HINSTANCE hInst, uint32_t width, uint32_t height);
	void Finalize();

	void PollEvents();
	void Show();
	void Hide();
	void ToggleFullScreen();

	bool ShouldClose() const { return m_ShouldClose; }

private:
	void RegisterWindow(HINSTANCE hInst);
	void CreateWindow(HINSTANCE hInst, uint32_t width, uint32_t height);

private:
	HWND m_hWnd = HWND();
	RECT m_WindowRect = RECT();

	bool m_ShouldClose = false;
	bool m_FullScreen = false;

};
