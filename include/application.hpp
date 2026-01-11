#pragma once

// TODO: add versioning script on source control (that gets updated on push)
#define SLATE_VERSION "V0.2 - 11/01/26"
namespace slate
{
class Window;
class Renderer;

class Application
{
public:
	void Initialize();
	void Run();
	void Shutdown();

	[[nodiscard]] Window& Window() const { return *m_Window; }
	[[nodiscard]] Renderer& Renderer() const { return *m_Renderer; }
private:
	slate::Window* m_Window{ nullptr };
	slate::Renderer* m_Renderer{ nullptr };
};

extern Application App;
} // namespace slate