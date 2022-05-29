#pragma once

struct WindowProps
{
	std::wstring Title;

	uint32_t Width;
	uint32_t Height;
};

class Window
{
public:
	Window();
	~Window();

	void Initialize(const WindowProps& properties);
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
	void RegisterWindow();
	void CreateWindow(const WindowProps& properties);

private:
	HWND m_hWnd = HWND();

	bool m_ShouldClose = false;
	bool m_FullScreen = false;

};
