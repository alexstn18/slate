#pragma once

namespace slate
{
	class SwapChain
	{
	public:
		void Initialize(HWND hWnd, u32 width, u32 height, u32 bufferCount);
		void Present(bool vSync);
		void Resize(u32 width, u32 height);

		u32 GetCurrentBackBufferIndex() const;
		ComPtr<ID3D12Resource> GetCurrentBackBuffer() const;
		ComPtr<ID3D12Resource> GetBackBuffer(u32 index) const;
		u32 GetBackBufferCount() const { return m_BufferCount; }

		DXGI_FORMAT GetFormat() const { return m_Format; }
		bool IsTearingSupported() const { return m_TearingSupported; }

		[[nodiscard]] ComPtr<IDXGISwapChain4> Get() const { return m_SwapChain; }
	protected:
		friend class Device;
	private:
		void CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue, HWND hWnd);
		void CreateBackBuffers();
		bool CheckForTearingSupport();
		void ReleaseBackBuffers();

		ComPtr<IDXGISwapChain4> m_SwapChain{ nullptr };
	
		std::vector<ComPtr<ID3D12Resource>> m_BackBuffers;

		DXGI_FORMAT m_Format = DXGI_FORMAT_R8G8B8A8_UNORM;
		u32 m_Width{ 0 };
		u32 m_Height{ 0 };
		u32 m_BufferCount{ 2 };

		bool m_TearingSupported{ false };
	};
}