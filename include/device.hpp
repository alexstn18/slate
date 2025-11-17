#pragma once

#include <d3d11.h>
#include <dxgi1_3.h>
#include <wrl.h>
#include <glm/vec4.hpp>

template<typename T>
using ComPtr = Microsoft::WRL::ComPtr<T>;
namespace slate
{
	class Device
	{
	public:
		Device(int width, int height);
		bool Initialize();
		bool OnResizeEvent();
		void Render();

		[[nodiscard]] glm::vec4 GetClearColor() const { return m_clearColor; }
		void SetClearColor(const glm::vec4& clearColor) { m_clearColor = clearColor; }
	private:
		bool CreateSwapchainResources();
		void DestroySwapchainResources();

		ComPtr<ID3D11Device> m_device{ nullptr };
		ComPtr<ID3D11DeviceContext> m_context{ nullptr };
		ComPtr<IDXGIFactory2> m_dxgiFactory{ nullptr };
		ComPtr<IDXGISwapChain1> m_swapChain{ nullptr };
		ComPtr<ID3D11RenderTargetView> m_renderTarget{ nullptr };

		int m_width{};
		int m_height{};

		glm::vec4 m_clearColor{ 0.0f, 0.0f, 0.0f, 1.0f };
	};
}