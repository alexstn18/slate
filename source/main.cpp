#include "application.hpp"

using namespace slate;

int main()
{
	App.Initialize();
	App.Run();
	App.Shutdown();

	return 0;
}