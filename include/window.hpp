#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <string_view>

namespace slate
{
class Window
{
public:
	Window(int width, int height, const std::string& title);
	~Window();

	bool Initialize();
	void ProcessMessages();
	[[nodiscard]] bool ShouldClose() const { return m_shouldClose; }
	[[nodiscard]] HWND GetHandle() const { return m_hwnd; }
	[[nodiscard]] int GetWidth() const { return m_width; }
	[[nodiscard]] int GetHeight() const { return m_height; }
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	int m_width{};
	int m_height{};
	std::string m_title{};
	std::string_view m_className{ "WindowClass" };
	HWND m_hwnd{nullptr};
	HINSTANCE m_hInstance{GetModuleHandle(nullptr)};

	bool m_shouldClose{false};
};
}
