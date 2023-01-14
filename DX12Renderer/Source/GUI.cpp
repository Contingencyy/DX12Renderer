#include "Pch.h"
#include "GUI.h"
#include "Graphics/Renderer.h"
#include "Graphics/Backend/CommandQueue.h"
#include "Graphics/Backend/CommandList.h"
#include "Graphics/Backend/SwapChain.h"
#include "Graphics/Backend/RenderBackend.h"

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

	auto device = RenderBackend::GetD3D12Device();
	device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_d3d12DescriptorHeap));

	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX12_Init(
		device, 3, DXGI_FORMAT_R8G8B8A8_UNORM, m_d3d12DescriptorHeap.Get(),
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

	auto commandList = RenderBackend::GetCommandList(D3D12_COMMAND_LIST_TYPE_DIRECT);
	
	commandList->Transition(Renderer::GetFinalColorOutput(), D3D12_RESOURCE_STATE_RENDER_TARGET);

	D3D12_CPU_DESCRIPTOR_HANDLE rtv = Renderer::GetFinalColorOutput().GetDescriptor(DescriptorType::RTV);
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = Renderer::GetFinalDepthOutput().GetDescriptor(DescriptorType::DSV);

	commandList->SetRenderTargets(1, &rtv, &dsv);
	ID3D12DescriptorHeap* descriptorHeap = m_d3d12DescriptorHeap.Get();
	commandList->GetGraphicsCommandList()->SetDescriptorHeaps(1, &descriptorHeap);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList->GetGraphicsCommandList().Get());

	/*if (ImGui::GetIO().ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		ImGui::UpdatePlatformWindows();
		ImGui::RenderPlatformWindowsDefault(NULL, (void*)commandList->GetGraphicsCommandList().Get());
	}*/

	RenderBackend::ExecuteCommandListAndWait(commandList);
}

void GUI::Finalize()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}
