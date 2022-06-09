#include "Pch.h"
#include "GUI.h"
#include "Application.h"
#include "Graphics/Renderer.h"
#include "Graphics/Device.h"
#include "Graphics/CommandQueue.h"
#include "Graphics/CommandList.h"
#include "Graphics/SwapChain.h"

#include <imgui/imgui.h>
#include <imgui/imgui_impl_win32.h>
#include <imgui/imgui_impl_dx12.h>

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
	auto device = renderer->GetDevice();
	device->CreateDescriptorHeap(heapDesc, m_d3d12DescriptorHeap);

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX12_Init(
		device->GetD3D12Device().Get(), renderer->GetSwapChain()->GetBackBufferCount(),
		DXGI_FORMAT_R8G8B8A8_UNORM, m_d3d12DescriptorHeap.Get(),
		m_d3d12DescriptorHeap->GetCPUDescriptorHandleForHeapStart(),
		m_d3d12DescriptorHeap->GetGPUDescriptorHandleForHeapStart()
	);
}

void GUI::Update(float deltaTime)
{
}

void GUI::BeginFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void GUI::EndFrame()
{
	ImGui::Render();

	auto renderer = Application::Get().GetRenderer();
	auto& commandQueue = renderer->m_CommandQueueDirect;
	auto commandList = commandQueue->GetCommandList();
	auto colorTarget = renderer->GetSwapChain()->GetColorTarget();
	auto depthBuffer = renderer->GetSwapChain()->GetDepthBuffer();

	auto rtv = colorTarget->GetDescriptorHandle();
	auto dsv = depthBuffer->GetDescriptorHandle();

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
