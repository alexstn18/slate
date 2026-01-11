#pragma once

//#include <glm/vec4.hpp>

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
		Renderer(u32 width, u32 height);
		virtual ~Renderer() = default;

		bool Initialize();
		void Update();
		void Render();

		[[nodiscard]] Adapter& GetAdapter() const noexcept { return *m_Adapter; }
		[[nodiscard]] Device& GetDevice() const noexcept { return *m_Device; }
		[[nodiscard]] CommandQueue& GetCommandQueue() const noexcept { return *m_CommandQueue; }
		[[nodiscard]] SwapChain& GetSwapChain() const noexcept { return *m_SwapChain; }
		[[nodiscard]] CommandList& GetCommandList() const noexcept { return *m_CommandList; }
		[[nodiscard]] RootSignature& GetRootSignature() const noexcept { return *m_RootSignature; }
		// d3d12 getters
		[[nodiscard]] ComPtr<IDXGIAdapter4> D3D12Adapter() const noexcept;
		[[nodiscard]] ComPtr<ID3D12Device2> D3D12Device() const noexcept;
		[[nodiscard]] ComPtr<ID3D12CommandQueue> D3D12CommandQueue() const noexcept;
		[[nodiscard]] ComPtr<IDXGISwapChain4> D3D12SwapChain() const noexcept;
		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList> D3D12CommandList() const noexcept;
		[[nodiscard]] ComPtr<ID3D12RootSignature> D3D12RootSignature() const noexcept;

		[[nodiscard]] u32 GetFrameCount() const noexcept { return m_NumBuffers; }

	private:
		void EnableDebugLayer();

		std::unique_ptr<Adapter>        m_Adapter{ nullptr };
		std::unique_ptr<Device>         m_Device{ nullptr };
		std::unique_ptr<CommandQueue>   m_CommandQueue{ nullptr };
		std::unique_ptr<SwapChain>		m_SwapChain{ nullptr };
		std::unique_ptr<DescriptorHeap> m_RTVDescriptorHeap{ nullptr };
		std::unique_ptr<DescriptorHeap> m_DSVDescriptorHeap{ nullptr };
		std::unique_ptr<CommandList>	m_CommandList{ nullptr };
		std::unique_ptr<RootSignature>  m_RootSignature{ nullptr };
	
		u32 m_Width{ 1280u };
		u32 m_Height{ 720u };
		static inline constexpr u32 m_NumBuffers{ 3u };

		//glm::vec4 m_ClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	};
}

