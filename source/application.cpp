#include "pch.hpp"
#include "application.hpp"
#include "window.hpp"
#include "renderer.hpp"

using namespace slate;

Application slate::App;

void Application::Initialize()
{
	// TODO: load W and H from config
	m_Window = new ::Window(1280, 720, "Slate");
	m_Renderer = new ::Renderer(m_Window->GetWidth(), m_Window->GetHeight());


	if (m_Window->Initialize()) log::Info("Win32 API Window has been initialized successfully!");
	else log::Critical("Win32 API Window has failed initialization.");

	if (m_Renderer->Initialize()) log::Info("D3D12 Device has been initialized successfully!");
	else log::Critical("D3D12 Device has failed initialization.");
}

void Application::Run()
{
	while (!m_Window->ShouldClose())
	{
		m_Window->ProcessMessages();
		m_Renderer->Update();
		m_Renderer->Render();
	}
}

void Application::Shutdown()
{
	if (m_Window->ShouldClose())
	{
		delete m_Renderer;
		delete m_Window;
	}
}
