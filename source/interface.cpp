#include "pch.hpp"
#include "interface.hpp"

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "window.hpp"
#include "renderer.hpp"

#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"

using namespace slate;

bool Interface::Initialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	// @TODO: enable mouse and keyboard controls here

	ImGui::StyleColorsDark();

	// @TODO: scaling

	ImGui_ImplWin32_Init(App.Window().GetHandle());

	auto& srvDescriptorHeap = App.Renderer().GetSRVDescriptorHeap();

	ImGui_ImplDX12_InitInfo info = {};
	info.Device = App.Renderer().D3D12Device().Get();
	info.CommandQueue = App.Renderer().D3D12CommandQueue().Get();
	info.NumFramesInFlight = App.Renderer().GetFrameCount();
	info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	info.SrvDescriptorHeap = srvDescriptorHeap.Get().Get();
	info.LegacySingleSrvCpuDescriptor = srvDescriptorHeap.GetCPUHandle(0);
	info.LegacySingleSrvGpuDescriptor = srvDescriptorHeap.GetGPUHandle(0);

	return ImGui_ImplDX12_Init(&info);
}

void Interface::Shutdown()
{
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();
}

void Interface::NewFrame()
{
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Interface::Update(float deltaTime)
{
	ImGui::ShowDemoWindow();
}

void Interface::Render()
{
	auto commandList = App.Renderer().D3D12CommandList();
	ImGui::Render();

	ID3D12DescriptorHeap* heaps[] = { App.Renderer().GetSRVDescriptorHeap().Get().Get() };
	commandList->SetDescriptorHeaps(1, heaps);

	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), commandList.Get());
}
