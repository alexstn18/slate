#pragma once
namespace slate {
	class Interface {
	public:
		bool Initialize();
		void Shutdown();
		void NewFrame();
		void Update(float deltaTime);
		void Render();
	};
}

