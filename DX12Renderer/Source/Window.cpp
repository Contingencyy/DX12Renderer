#include "Pch.h"
#include "Window.h"
#include "Application.h"
#include "Renderer.h"

RECT windowRect = RECT();
RECT clientRect = RECT();

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (Application::Get().IsInitialized())
    {
        switch (message)
        {
            case WM_SYSKEYDOWN:
            case WM_KEYDOWN:
            {
                bool alt = (::GetAsyncKeyState(VK_MENU) & 0x8000) != 0;
                switch (wParam)
                {
                case VK_ESCAPE:
                    ::PostQuitMessage(0);
                    break;
                case VK_RETURN:
                    if (alt)
                    {
                case VK_F11:
                    Application::Get().GetWindow()->ToggleFullScreen();
                    }
                    break;
                case 0x56:
                    Application::Get().GetRenderer()->ToggleVSync();
                    break;
                }
            }
            break;

            case WM_SIZE:
            case WM_SIZING:
            {
                ::GetClientRect(Application::Get().GetWindow()->GetHandle(), &clientRect);
                auto window = Application::Get().GetWindow();

                Application::Get().GetRenderer()->Resize(window->GetWidth(), window->GetHeight());
            }
            break;

            case WM_DESTROY:
                ::PostQuitMessage(0);
                break;
            default:
                return ::DefWindowProcW(hWnd, message, wParam, lParam);
        }
    }
    else
    {
        return ::DefWindowProcW(hWnd, message, wParam, lParam);
    }

    return 0;
}

Window::Window()
{
}

Window::~Window()
{
}

void Window::Initialize(HINSTANCE hInst, uint32_t width, uint32_t height)
{
	RegisterWindow(hInst);
	CreateWindow(hInst, width, height);
}

void Window::Finalize()
{
}

void Window::PollEvents()
{
    MSG msg = {};
    while (::PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) != NULL)
    {
        if (msg.message == WM_QUIT)
        {
            m_ShouldClose = true;
            break;
        }

        ::TranslateMessage(&msg);
        ::DispatchMessage(&msg);
    }
}

void Window::Show()
{
    ::ShowWindow(m_hWnd, 1);
}

void Window::Hide()
{
    ::ShowWindow(m_hWnd, 0);
}

void Window::ToggleFullScreen()
{
    m_FullScreen = !m_FullScreen;
    if (m_FullScreen)
    {
        ::GetWindowRect(m_hWnd, &windowRect);

        UINT windowStyle = WS_OVERLAPPEDWINDOW & ~(WS_CAPTION | WS_SYSMENU |
            WS_THICKFRAME | WS_MINIMIZEBOX | WS_MAXIMIZEBOX);

        ::SetWindowLongW(m_hWnd, GWL_STYLE, windowStyle);

        HMONITOR hMonitor = ::MonitorFromWindow(m_hWnd, MONITOR_DEFAULTTONEAREST);
        MONITORINFOEX monitorInfo = {};
        monitorInfo.cbSize = sizeof(MONITORINFOEX);
        ::GetMonitorInfo(hMonitor, &monitorInfo);
        ::SetWindowPos(m_hWnd, HWND_TOP, monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right - monitorInfo.rcMonitor.left,
            monitorInfo.rcMonitor.bottom - monitorInfo.rcMonitor.top,
            SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_MAXIMIZE);
    }
    else
    {
        ::SetWindowLong(m_hWnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);

        ::SetWindowPos(m_hWnd, HWND_NOTOPMOST, windowRect.left,
            windowRect.top, windowRect.right - windowRect.left,
            windowRect.bottom - windowRect.top, SWP_FRAMECHANGED | SWP_NOACTIVATE);

        ::ShowWindow(m_hWnd, SW_NORMAL);
    }
}

uint32_t Window::GetWidth() const
{
    return clientRect.right - clientRect.left;
}

uint32_t Window::GetHeight() const
{
    return clientRect.bottom - clientRect.top;
}

void Window::RegisterWindow(HINSTANCE hInst)
{
    const wchar_t* windowClassName = L"DX12RendererWindow";
    WNDCLASSEXW windowClass = {};

    windowClass.cbSize = sizeof(WNDCLASSEX);
    windowClass.style = CS_HREDRAW | CS_VREDRAW;
    windowClass.lpfnWndProc = &WndProc;
    windowClass.cbClsExtra = 0;
    windowClass.cbWndExtra = 0;
    windowClass.hInstance = hInst;
    windowClass.hIcon = ::LoadIcon(hInst, 0);
    windowClass.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    windowClass.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    windowClass.lpszMenuName = NULL;
    windowClass.lpszClassName = windowClassName;
    windowClass.hIconSm = ::LoadIcon(hInst, NULL);

    static ATOM atom = ::RegisterClassExW(&windowClass);
    assert(atom > 0);
}

void Window::CreateWindow(HINSTANCE hInst, uint32_t width, uint32_t height)
{
    int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
    int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);

    windowRect = { 0, 0, static_cast<LONG>(width), static_cast<LONG>(height) };
    ::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);

    int windowWidth = windowRect.right - windowRect.left;
    int windowHeight = windowRect.bottom - windowRect.top;

    int windowX = std::max<int>(0, (screenWidth - windowWidth) / 2);
    int windowY = std::max<int>(0, (screenHeight - windowHeight) / 2);

    m_hWnd = ::CreateWindowExW(NULL, L"DX12RendererWindow", L"DX12 Renderer", WS_OVERLAPPEDWINDOW,
        windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInst, nullptr);

    assert(m_hWnd && "Failed to create window\n");

    ::GetClientRect(m_hWnd, &clientRect);
}
