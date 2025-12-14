#include "pch.hpp"
#include "window.hpp"

using namespace slate;

Window::Window(int width, int height, const std::string& title)
	: m_width{width}
	, m_height{height}
	, m_title{title}
{
}

Window::~Window()
{
	if (m_hwnd)
	{
		DestroyWindow(m_hwnd);
		UnregisterClassA(m_className.data(), m_hInstance);
	}
}

bool Window::Initialize()
{
	WNDCLASSA wnd = {};
	wnd.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wnd.lpfnWndProc = &WndProc;
	wnd.cbClsExtra = 0;
	wnd.cbWndExtra = 0;
	wnd.hInstance = GetModuleHandle(nullptr);
	wnd.hIcon = nullptr;
	wnd.hCursor = LoadCursor(nullptr, IDC_ARROW);
	wnd.hbrBackground = nullptr;
	wnd.lpszMenuName = nullptr;
	wnd.lpszClassName = m_className.data();
	if (!RegisterClassA(&wnd)) return false;

	const DWORD exStyle = WS_EX_APPWINDOW;
	const DWORD style = WS_OVERLAPPEDWINDOW | WS_VISIBLE;
	RECT windowRect = { 0,0,m_width,m_height };
	AdjustWindowRectEx(&windowRect, style, FALSE, exStyle);
	m_hwnd = CreateWindowExA(exStyle, m_className.data(), 
							m_title.c_str(), style, 
							0, 0, 
							windowRect.right - windowRect.left, 
							windowRect.bottom - windowRect.top,
							nullptr, nullptr, 
							wnd.hInstance, this);
	
	if (!m_hwnd) return false;

	return true;
}

void Window::ProcessMessages()
{
	MSG msg;
	while (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
}

LRESULT Window::WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	Window* self;
	if (uMsg == WM_NCCREATE)
	{
		CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
		self = reinterpret_cast<Window*>(cs->lpCreateParams);

		SetWindowLongPtr(hWnd, GWLP_USERDATA, (LONG_PTR)self);
	}
	else
	{
		self = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
	}

	if (self) return self->HandleMessage(hWnd, uMsg, wParam, lParam);

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

LRESULT Window::HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
		case WM_PAINT:
		{
			ValidateRect(hWnd, nullptr);
			break;
		}
		case WM_KEYDOWN:
		{
			switch (wParam)
			{
				case VK_ESCAPE:
				{
					m_shouldClose = true;
					break;
				}
			}
			break;
		}
		case WM_SIZE:
		{
			m_width = LOWORD(lParam);
			m_height = HIWORD(lParam);
			// TODO: add resize later
			break;
		}
		case WM_SYSCOMMAND:
		{
			switch (wParam)
			{
				case SC_SCREENSAVE:
				case SC_MONITORPOWER:
				{
					return 0;
				}
			}
			break;
		}
		case WM_DESTROY:
		{
			PostQuitMessage(0);
			m_shouldClose = true;
			break;
		}
		case WM_CLOSE:
		{
			m_shouldClose = true;
			break;
		}
	}
	return DefWindowProcA(hWnd, uMsg, wParam, lParam);
}