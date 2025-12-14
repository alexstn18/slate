#pragma once

#include "d3d12/include/d3d12.h"
#include "d3d12/include/d3dx12.h"

#include <dxgi1_6.h>
#include <wrl.h>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include <chrono>

struct Vertex
{
	glm::vec3 Position;
	glm::vec4 Color;
};

namespace slate
{
	class Device
	{
	public:
		Device(int width, int height);
		~Device();
		bool Initialize();
		bool OnResizeEvent();
		void Update();
		void Render();

		[[nodiscard]] ComPtr<ID3D12Device2> GetDevice() const { return m_device; }
		[[nodiscard]] glm::vec4 GetClearColor() const { return m_clearColor; }
		void SetClearColor(const glm::vec4& clearColor) { m_clearColor = clearColor; }
	private:
		void CreateDevice(ComPtr<IDXGIAdapter4> adapter);
		void CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type);
		void CreateSwapChain(HWND hWnd, uint32_t bufferCount);
		void CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors);
		ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type);
		ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator, D3D12_COMMAND_LIST_TYPE type);
		ComPtr<ID3D12Fence> CreateFence();
		HANDLE CreateEventHandle();
		void CreateRootSignature();
		void CompileTriangleShaders();
		void CreateVertexBuffer();
		void CreateDepthStencil();

		uint64_t Signal(ComPtr<ID3D12Fence> fence, uint64_t& fenceValue);
		void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration);
		void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent);
		void UpdateRenderTargetViews();
		bool CheckForTearingSupport();
		void EnableDebugLayer();
		ComPtr<IDXGIAdapter4> GetAdapter(bool useWarp);

		static constexpr uint8_t m_numFrames{ 3 }; // controls the number of backbuffer surfaces for swapchain, can't be < 2 when using flip presentation model

		ComPtr<ID3D12Device2> m_device{ nullptr };
		ComPtr<ID3D12CommandQueue> m_commandQueue{ nullptr };
		
		ComPtr<IDXGISwapChain4> m_swapChain{ nullptr }; // responsible for presenting the rendered image to the window
		ComPtr<IDXGIFactory7> m_dxgiFactory{ nullptr };
		ComPtr<ID3D12Resource> m_backBuffers[m_numFrames]{ nullptr }; // all buffer and texture resources use ID3D12Resource interface
		ComPtr<ID3D12GraphicsCommandList> m_commandList{ nullptr }; // GPU commands are recorded into this

		/// Command Allocators
		// backing memory for recording the GPU commands into a command list, stores the reference to the command allocators
		// command allocator can't be reused unless the recorded old commands have finished executing on GPU
		// attempting to reset command allocator before the queue is finished will result in an error by debug layer
		// there must be at least one command allocator per frame that is "in-flight" (at least one per back buf of swapchain)
		ComPtr<ID3D12CommandAllocator> m_commandAllocators[m_numFrames]{ nullptr }; 
		
		/// Render Target View Descriptor Heap
		// back buf textures of swapchain are described using RTV (render target view)
		// RTV describes the location of the texture resource in GPU memory, the dimensions of the tex and the format of the tex
		// RTV is used to clear the back bufs of the render target
		// in D3D12 RTV descriptor heap is an array of descriptors (views)
		// a view simply describes a resource that resides in GPU memory
		//
		/// More about descriptors
		// descriptor describes a resource and a descriptor is needed to describe each back buf tex
		// size of descriptor is GPU vendor specific 
		ComPtr<ID3D12DescriptorHeap> m_rtvDescriptorHeap{ nullptr }; // used to store the descriptor heap that contains the render target views for the swapchain back bufs
		ComPtr<ID3D12DescriptorHeap> m_DSVHeap{ nullptr }; // depth stencil view 

		ComPtr<ID3D12RootSignature> m_rootSignature{ nullptr };
		ComPtr<ID3D12PipelineState> m_pipelineState{ nullptr };

		ComPtr<ID3D12Resource> m_vertexBuffer{ nullptr };
		ComPtr<ID3D12Resource> m_indexBuffer{ nullptr };
		ComPtr<ID3D12Resource> m_depthStencilBuffer{ nullptr };
		D3D12_VERTEX_BUFFER_VIEW m_vertexBufferView;
		D3D12_INDEX_BUFFER_VIEW m_indexBufferView;

		UINT m_rtvDescriptorSize{};

		// current back buf idx depends on the flip model of the swap chain and it may not be sequential
		UINT m_currentBackBufferIndex{}; // stores the idx of the current back buf of the swapchain 

		CD3DX12_VIEWPORT m_viewport{};
		CD3DX12_RECT m_scissorRect{};
		uint32_t m_width{};
		uint32_t m_height{};

		// used to verify results of rendering technique on old GPU
		bool m_useWARP{ false }; // windows advanced rasterization platform -- gives more advanced rendering features on older GPUs 
		bool m_isInitialized{ false };

		/// Fences
		// For each rendered frame that could be "in-flight" on the command queue, the fence value that was used to signal the command queue needs to be tracked
		// to guarantee that any resources that are still being referenced by the command queue are not overwritten
		//
		// If the fence objects completed value has not reached the fence value specified for the frame, then the CPU thread will stall until the fence value is reached
		ComPtr<ID3D12Fence> m_fence{ nullptr };
		uint64_t m_fenceValue{ 0 }; // next fence value to signal the command queue next
		uint64_t m_frameFenceValues[m_numFrames]{}; // keeps track of the fence values that were used to signal the command queue for a particular frame
		HANDLE m_fenceEvent; // handle to an OS event object that will be used to receive the notification that the fence has reached a specific value

		/// VSync
		// controls whether swap chain's present method should wait for the next vertical refresh before presenting
		// by default the present method will block until the next vertical refresh of the screen
		//
		// caps framerate of app to the refresh rate of the screen
		// 
		// setting m_vsync to false will cause the swapchain to present the rendered image to the screen as fast as possible which will allow the app
		// to render at an unthrottled frame rate but may cause visual artifacts in the form of screen tearing (eliminated by G-Sync or FreeSync)
		bool m_vsync{ true }; 
		bool m_tearingSupported{ false };
		bool m_fullscreen{ false };

		glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
		glm::mat4 m_mvpMatrix{};
	};
}