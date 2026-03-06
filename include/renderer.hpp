#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "rendering/render_components.hpp"

#include <d3d12ma/D3D12MemAlloc.h>

namespace slate {
	class Adapter;
	class Device;
	class CommandQueue;
	class SwapChain;
	class DescriptorHeap;
	class CommandList;
	class RenderTarget;
	class RootSignature;
	class PipelineStateObject;
	class Model;
	class Interface;
	class StructuredBuffer;

	class Renderer {
	public:
		Renderer(u32 width, u32 height);
		virtual ~Renderer();

		bool Initialize();
		void Shutdown();
		void Update();
		void Render();

		void TrackUpload(ComPtr<ID3D12Resource> resource, D3D12MA::Allocation* allocation = nullptr);
		void FlushUploads();
		[[nodiscard]] u32 IncrementTextureCount() noexcept { return m_NextSRVIndex++; }

		[[nodiscard]] Adapter& GetAdapter() const noexcept { return *m_Adapter; }
		[[nodiscard]] Device& GetDevice() const noexcept { return *m_Device; }
		[[nodiscard]] CommandQueue& GetCommandQueue() const noexcept { return *m_CommandQueue; }
		[[nodiscard]] SwapChain& GetSwapChain() const noexcept { return *m_SwapChain; }
		[[nodiscard]] std::shared_ptr<CommandList> GetCommandList() const noexcept { return m_CommandList; }
		[[nodiscard]] RenderTarget& GetRenderTarget() const noexcept { return *m_RenderTarget; }
		[[nodiscard]] DescriptorHeap& GetSRVDescriptorHeap() const noexcept { return *m_SRVDescriptorHeap; }

		// d3d12 getters
		[[nodiscard]] ComPtr<IDXGIAdapter4> D3D12Adapter() const noexcept;
		[[nodiscard]] ComPtr<ID3D12Device2> D3D12Device() const noexcept;
		[[nodiscard]] ComPtr<ID3D12CommandQueue> D3D12CommandQueue() const noexcept;
		[[nodiscard]] ComPtr<IDXGISwapChain4> D3D12SwapChain() const noexcept;
		[[nodiscard]] ComPtr<ID3D12GraphicsCommandList> D3D12CommandList() const noexcept;

		// d3d12ma
		[[nodiscard]] D3D12MA::Allocator& D3D12MA_Allocator() const noexcept { return *m_Allocator; }

		[[nodiscard]] u32 GetFrameCount() const noexcept { return m_NumBuffers; }

		[[nodiscard]] const std::vector<ComPtr<ID3D12CommandAllocator>>& GetCommandAllocators() const noexcept { return m_CommandAllocators; }
		[[nodiscard]] std::vector<Light>& GetLights() noexcept { return m_Lights; }
		[[nodiscard]] Camera& GetCamera() noexcept { return m_Camera; }
		[[nodiscard]] Model& GetModel() noexcept { return *m_Model; }
	private:
		struct PendingUpload {
			ComPtr<ID3D12Resource> Resource;
			D3D12MA::Allocation* Allocation = nullptr;
		};

		void EnableDebugLayer();
		void InitializeCommandAllocators();
		void UpdateRenderTargetViews();
		void CreateDepthStencil();
		void CreateRootSignature();
		void CompileShaders();

		std::unique_ptr<Adapter>        m_Adapter{ nullptr };
		std::unique_ptr<Device>         m_Device{ nullptr };
		std::unique_ptr<CommandQueue>   m_CommandQueue{ nullptr };
		std::unique_ptr<SwapChain>		m_SwapChain{ nullptr };
		std::unique_ptr<DescriptorHeap> m_RTVDescriptorHeap{ nullptr };
		std::unique_ptr<DescriptorHeap> m_DSVDescriptorHeap{ nullptr };
		std::unique_ptr<DescriptorHeap> m_SRVDescriptorHeap{ nullptr };
		std::shared_ptr<CommandList>	m_CommandList{ nullptr };
		std::shared_ptr<Model>			m_Model{ nullptr };
		std::unique_ptr<RenderTarget>   m_RenderTarget{ nullptr };
		std::unique_ptr<RootSignature> m_RootSignature{ nullptr };
		std::unique_ptr<PipelineStateObject> m_PipelineState{ nullptr };
		std::unique_ptr<Interface> m_Interface{ nullptr };
		std::unique_ptr<StructuredBuffer> m_LightBuffer{ nullptr };

		D3D12MA::Allocator* m_Allocator{ nullptr };
		D3D12MA::Allocation* m_DepthStencilAllocation{ nullptr };

		D3D12_VIEWPORT m_Viewport;
		D3D12_RECT m_ScissorRect;
		ComPtr<ID3D12Resource> m_DepthStencilBuffer{ nullptr };

		std::vector<ComPtr<ID3D12CommandAllocator>> m_CommandAllocators{nullptr};
		std::vector<PendingUpload> m_PendingUploads;

		u32 m_Width{ 1280u };
		u32 m_Height{ 720u };
		u32 m_NextSRVIndex{ 1u };
		static inline constexpr u32 m_NumBuffers{ 3u };

		glm::vec4 m_ClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };

		Camera m_Camera;
		std::vector<Light> m_Lights;
	};
}

