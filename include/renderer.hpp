#pragma once

namespace slate {
	class Adapter;
	class Device;
	class CommandQueue;
	class SwapChain;
	class DescriptorHeap;
	class CommandList;
	class RootSignature;

	class Renderer {
	public:
		Renderer() = default;
		virtual ~Renderer();

		void Initialize();
	private:
		void EnableDebugLayer();

		std::unique_ptr<Adapter> m_Adapter{ nullptr };
		std::unique_ptr<Device> m_Device{ nullptr };
		std::unique_ptr<CommandQueue> m_CommandQueue{ nullptr };
		std::unique_ptr<SwapChain> m_SwapChain{ nullptr };
		std::unique_ptr<DescriptorHeap> m_DescriptorHeap{ nullptr };
		std::unique_ptr<CommandList> m_CommandList{ nullptr };
		std::unique_ptr<RootSignature> m_RootSignature{ nullptr };
	};
}

