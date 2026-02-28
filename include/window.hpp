#pragma once
#include <string>
#include <string_view>

namespace slate
{
class Window
{
public:
	Window(i32 width, i32 height, const std::string& title);
	~Window();

	bool Initialize();
	void ProcessMessages();
	[[nodiscard]] bool ShouldClose() const { return m_shouldClose; }
	[[nodiscard]] HWND GetHandle() const { return m_hwnd; }
	[[nodiscard]] i32 GetWidth() const { return m_width; }
	[[nodiscard]] i32 GetHeight() const { return m_height; }
private:
	static LRESULT CALLBACK WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	LRESULT HandleMessage(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

	i32 m_width{};
	i32 m_height{};
	std::string m_title{};
	std::string_view m_className{ "WindowClass" };
	HWND m_hwnd{ nullptr };
	HINSTANCE m_hInstance{ GetModuleHandle( nullptr ) };

	bool m_shouldClose{ false };
};
}
