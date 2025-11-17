#include "application.hpp"
#include "window.hpp"
#include "device.hpp"
#include "log.hpp"

using namespace slate;

Application slate::App;

void Application::Initialize()
{
	// TODO: load W and H from config
	m_window = new ::Window(1280, 720, "Slate");
	m_device = new ::Device(m_window->GetWidth(), m_window->GetHeight());


	if (m_window->Initialize()) log::Info("Win32 API Window has been initialized successfully!");
	else log::Critical("Win32 API Window has failed initialization.");

	if (m_device->Initialize()) log::Info("D3D11 Device has been initialized successfully!");
	else log::Critical("D3D11 Device has failed initialization.");
}

void Application::Run()
{
	while (!m_window->ShouldClose())
	{
		m_window->ProcessMessages();
		m_device->Render();
	}
}

void Application::Shutdown()
{
	if (m_window->ShouldClose())
	{
		delete m_window;
	}
}
