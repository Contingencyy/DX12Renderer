#include "Pch.h"
#include "Application.h"

void CreateConsole()
{
	if (!AllocConsole())
	{
		DWORD err = GetLastError();
		assert(false && err);
		return;
	}

	FILE* fDummy;
	freopen_s(&fDummy, "CONOUT$", "w", stdout);
	freopen_s(&fDummy, "CONOUT$", "w", stderr);
	freopen_s(&fDummy, "CONIN$", "r", stdin);
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PWSTR pCmdLine, int nCmdShow)
{
	CreateConsole();
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	Application::Create();

	Application::Get().Initialize(hInstance, 1280, 720);
	Application::Get().Run();
	Application::Get().Finalize();

	Application::Destroy();

	return 0;
}
