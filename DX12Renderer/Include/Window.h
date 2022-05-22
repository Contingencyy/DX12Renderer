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

	uint32_t GetWidth() const;
	uint32_t GetHeight() const;

	bool ShouldClose() const { return m_ShouldClose; }
	HWND GetHandle() const { return m_hWnd; }

private:
	void RegisterWindow(HINSTANCE hInst);
	void CreateWindow(HINSTANCE hInst, uint32_t width, uint32_t height);

private:
	HWND m_hWnd = HWND();

	bool m_ShouldClose = false;
	bool m_FullScreen = false;

};
