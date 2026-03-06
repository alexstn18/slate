#include "pch.hpp"
#include "interface.hpp"

#include "imgui/imgui.h"
#include "imgui/ImReflect.hpp"
#include "imgui/ImGuizmo.h"
#include "imgui/backends/imgui_impl_win32.h"
#include "imgui/backends/imgui_impl_dx12.h"

#include "window.hpp"
#include "renderer.hpp"

#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"

#include "rendering/render_components.hpp"
#include "rendering/model.hpp"
#include "rendering/structured_buffer.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtc/type_ptr.hpp>

using namespace slate;

bool Interface::Initialize()
{
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); ( void )io;

	ImGui::StyleColorsDark();

	// @TODO: scaling

	ImGui_ImplWin32_Init( App.Window().GetHandle() );

	auto& srvDescriptorHeap = App.Renderer().GetSRVDescriptorHeap();

	ImGui_ImplDX12_InitInfo info = {};
	info.Device = App.Renderer().D3D12Device().Get();
	info.CommandQueue = App.Renderer().D3D12CommandQueue().Get();
	info.NumFramesInFlight = App.Renderer().GetFrameCount();
	info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	info.DSVFormat = DXGI_FORMAT_D32_FLOAT;
	info.SrvDescriptorHeap = srvDescriptorHeap.Get().Get();
	info.LegacySingleSrvCpuDescriptor = srvDescriptorHeap.GetCPUHandle( 0u );
	info.LegacySingleSrvGpuDescriptor = srvDescriptorHeap.GetGPUHandle( 0u );

	return ImGui_ImplDX12_Init( &info );
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
	ImGuizmo::BeginFrame();
}

void Interface::Update(float)
{
	ImGuizmo::SetOrthographic(false);
	ImGuizmo::SetDrawlist(ImGui::GetBackgroundDrawList());

	ImGuiIO& io = ImGui::GetIO();
	if (io.DisplaySize.x <= 0 || io.DisplaySize.y <= 0) return;
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	auto& cam = App.Renderer().GetCamera();
	const glm::mat4& view = cam.GetView();
	const glm::mat4& proj = cam.GetProjection(io.DisplaySize.x, io.DisplaySize.y);

	auto& transform = App.Renderer().GetModel().GetTransform();
	glm::mat4 matrix = transform.World();
	
	static ImGuizmo::OPERATION manipOperation = ImGuizmo::TRANSLATE;

	if(ImGuizmo::Manipulate(glm::value_ptr(view), glm::value_ptr(proj), manipOperation, ImGuizmo::WORLD, glm::value_ptr(matrix))) 
	{
		transform.SetFromMatrix(matrix);
	}
	
	bool dirty = false;
	ImGui::Begin("slate-dbg-wnd");
	{
		if (ImGui::RadioButton("Translate", manipOperation == ImGuizmo::TRANSLATE)) manipOperation = ImGuizmo::TRANSLATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Rotate", manipOperation == ImGuizmo::ROTATE))    manipOperation = ImGuizmo::ROTATE;
		ImGui::SameLine();
		if (ImGui::RadioButton("Scale", manipOperation == ImGuizmo::SCALE))     manipOperation = ImGuizmo::SCALE;

		ImReflect::Input("Camera", cam);
		ImGui::SliderAngle("yaw", &cam.yaw, -180.f, 180.f);
		ImGui::SliderAngle("pitch", &cam.pitch, -89.f, 89.f);
		ImGui::SliderAngle("fovY", &cam.fovY, 10.f, 170.f);

		for (auto& light : App.Renderer().GetLights()) {
			auto response = ImReflect::Input("Light", light);
			if (response.get<Light>().is_changed()) dirty = true;
			if (ImGui::ColorEdit3("Color", &light.Color[0])) dirty = true;
			const char* types[] = { "Directional", "Point" };
			i32 t = (i32)light.type;
			if (ImGui::Combo("Type", &t, types, 2)) {
				light.type = (slate::Light::Type)t;
				dirty = true;
			}
		}
	}

	if (dirty)
	{
		auto& lights = App.Renderer().GetLights();
		App.Renderer().GetLightBuffer().SetData(lights.data(), lights.size() * sizeof(Light));
	}

	ImGui::End();
}

void Interface::Render()
{
	auto commandList = App.Renderer().D3D12CommandList();
	ImGui::Render();

	ID3D12DescriptorHeap* heaps[] = { App.Renderer().GetSRVDescriptorHeap().Get().Get() };
	commandList->SetDescriptorHeaps( 1u, heaps );

	ImGui_ImplDX12_RenderDrawData( ImGui::GetDrawData(), commandList.Get() );
}