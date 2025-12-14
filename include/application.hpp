#pragma once

// TODO: add versioning script on source control (that gets updated on push)
#define SLATE_VERSION "V0.1 - 16/11/25"
namespace slate
{
class Window;
class Device;

class Application
{
public:
	void Initialize();
	void Run();
	void Shutdown();

	[[nodiscard]] Window& Window() const { return *m_window; }
	[[nodiscard]] Device& Device() const { return *m_device; }
private:
	slate::Window* m_window{ nullptr };
	slate::Device* m_device{ nullptr };
};

extern Application App;
} // namespace slate