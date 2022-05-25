#include "Pch.h"
#include "Application.h"

int main(int argc, char* argv[])
{
	SetThreadDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

	Application::Create();

	Application::Get().Initialize(GetModuleHandle(NULL), 1280, 720);
	Application::Get().Run();
	Application::Get().Finalize();

	Application::Destroy();

	return 0;
}
