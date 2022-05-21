#include "Pch.h"
#include "GUI.h"
#include "Application.h"
#include "Renderer.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "imgui/imgui.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

GUI::GUI()
{
}

GUI::~GUI()
{
}

void GUI::Initialize(HWND hWnd)
{
	// Set up ImGui context, styles and flags
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
	//io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;

	ImGui::StyleColorsDark();

	/*ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}*/

	// Initialize DX12 backend for ImGui
	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	heapDesc.NumDescriptors = 1;
	heapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	heapDesc.NodeMask = 0;

	auto renderer = Application::Get().GetRenderer();
	DX_CALL(renderer->m_d3d12Device->CreateDescriptorHeap(
		&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap)
	));

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX12_Init(
		renderer->m_d3d12Device.Get(), renderer->s_BackBufferCount,
		DXGI_FORMAT_R8G8B8A8_UNORM, m_d3d12DescriptorHeap.Get(),
		m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);
}

void GUI::Update(float deltaTime)
{
}

void GUI::Render()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	ImGui::SetNextWindowSize(ImVec2(200, 100));
	ImGui::Begin("Profiler");

	float lastFrameDuration = Application::Get().GetLastFrameTime().count() * 1000.0f;
	Renderer::RenderSettings renderSettings = Application::Get().GetRenderer()->GetRenderSettings();

	ImGui::Text("Resolution: %ux%u", renderSettings.Resolution.x, renderSettings.Resolution.y);
	ImGui::Text("VSync: %s", renderSettings.VSync ? "true" : "false");
	ImGui::Text("Frametime: %.3f ms", lastFrameDuration);
	ImGui::Text("FPS: %u", static_cast<uint32_t>(1000.0f / lastFrameDuration));
	ImGui::End();

	ImGui::Render();

	auto renderer = Application::Get().GetRenderer();
	auto& commandQueue = renderer->m_CommandQueueDirect;
	auto commandList = commandQueue->GetCommandList();

	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(renderer->m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]->GetCPUDescriptorHandleForHeapStart(),
		renderer->m_CurrentBackBufferIndex, renderer->m_DescriptorSize[D3D12_DESCRIPTOR_HEAP_TYPE_RTV]);
	CD3DX12_CPU_DESCRIPTOR_HANDLE dsv(renderer->m_d3d12DescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_DSV]->GetCPUDescriptorHandleForHeapStart());

	commandList->SetRenderTargets(1, &rtv, &dsv);
	ID3D12DescriptorHeap* descriptorHeap = m_d3d12DescriptorHeap.Get();
	commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &descriptorHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetGraphicsCommandList().Get());

	/*if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(NULL, (void*)commandList->GetGraphicsCommandList().Get());
	}*/

	uint64_t fenceValue = commandQueue->ExecuteCommandList(commandList);
	commandQueue->WaitForFenceValue(fenceValue);
}

void GUI::Finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
