#pragma once

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "rendering/render_components.hpp"

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
	class VertexBuffer;
	class IndexBuffer;
	class Model;
	class Interface;

	class Renderer {
	public:
		Renderer(u32 width, u32 height);
		virtual ~Renderer() = default;

		bool Initialize();
		void Shutdown();
		void Update();
		void Render();

		void TrackUpload(ComPtr<ID3D12Resource> resource);
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

		[[nodiscard]] u32 GetFrameCount() const noexcept { return m_NumBuffers; }

		[[nodiscard]] const std::vector<ComPtr<ID3D12CommandAllocator>>& GetCommandAllocators() const noexcept { return m_CommandAllocators; }
	private:
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
		std::unique_ptr<VertexBuffer> m_VertexBuffer{ nullptr };
		std::unique_ptr<IndexBuffer> m_IndexBuffer{ nullptr };
		std::unique_ptr<Interface> m_Interface{ nullptr };

		D3D12_VIEWPORT m_Viewport;
		D3D12_RECT m_ScissorRect;
		ComPtr<ID3D12Resource> m_DepthStencilBuffer{ nullptr };

		std::vector<ComPtr<ID3D12CommandAllocator>> m_CommandAllocators{nullptr};
		std::vector<ComPtr<ID3D12Resource>> m_PendingUploads{ nullptr };

		u32 m_Width{ 1280u };
		u32 m_Height{ 720u };
		u32 m_NextSRVIndex{ 1u };
		static inline constexpr u32 m_NumBuffers{ 3u };

		glm::vec4 m_ClearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		glm::mat4 m_VPMatrix{};

		struct LightInfo {
			glm::vec3 Color;
			float _pad0;
			glm::vec3 Position;
			float _pad1;
			glm::vec3 Direction;
			float Intensity;
			float _pad2[4];
			void LightToLightInfo(const Light& light);
		};

		struct Constants {
			glm::mat4 Model;
			glm::mat4 VP;
			//LightInfo lightInfo;
		}m_Constants;

		Light m_Light;
	};
}

