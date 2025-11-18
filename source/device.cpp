#include "device.hpp"
#include "application.hpp"
#include "window.hpp"
#include "log.hpp"

using namespace slate;

Device::Device(int width, int height)
	: m_width{width}
	, m_height{height}
{
}

Device::~Device()
{
	m_context->Flush();
	DestroySwapchainResources();
	m_swapChain.Reset();
	m_dxgiFactory.Reset();
	m_context.Reset();
	m_device.Reset();
}

bool Device::Initialize()
{
	if (FAILED(CreateDXGIFactory1(IID_PPV_ARGS(&m_dxgiFactory))))
	{
		log::Error("Unable to create DXGIFactory");
		return false;
	}

	constexpr D3D_FEATURE_LEVEL deviceFeatureLevel = D3D_FEATURE_LEVEL_11_0;
	if (FAILED(D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, &deviceFeatureLevel, 1, D3D11_SDK_VERSION, &m_device, nullptr, &m_context)))
	{
		log::Error("Failed to create D3D11Device and D3D11DeviceContext");
		return false;
	}

	DXGI_SWAP_CHAIN_DESC1 scd = {};
	scd.Width = m_width;
	scd.Height = m_height;
	scd.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // color components from 0 to 255
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.BufferCount = 2; // double buffering
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD; // avoid flickering?
	scd.Scaling = DXGI_SCALING_STRETCH;
	scd.Flags = {};

	DXGI_SWAP_CHAIN_FULLSCREEN_DESC scFullscreenDescriptor = {};
	scFullscreenDescriptor.Windowed = true; // could (or should) be changed to fullscreen

	if (FAILED(m_dxgiFactory->CreateSwapChainForHwnd(m_device.Get(), App.Window().GetHandle(), &scd, &scFullscreenDescriptor, nullptr, &m_swapChain)))
	{
		log::Error("Failed to create swapchain");
		return false;
	}

	if (!CreateSwapchainResources())
	{
		log::Error("Unable to create swapchain resources on initialization of program");
		return false;
	}

	return true;
}

bool Device::OnResizeEvent()
{
	m_context->Flush();
	DestroySwapchainResources();

	if (FAILED(m_swapChain->ResizeBuffers(0, m_width, m_height, DXGI_FORMAT_B8G8R8A8_UNORM, 0)))
	{
		log::Error("Failed to recreate/resize SwapChain buffers (OnResizeEvent)");
		return false;
	}

	if (!CreateSwapchainResources())
	{
		log::Error("Failed to create SwapChain resources (OnResizeEvent)");
		return false;
	}

	return true;
}

void Device::Render()
{
	D3D11_VIEWPORT viewport = {};
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(App.Window().GetWidth());
	viewport.Height = static_cast<float>(App.Window().GetHeight());
	viewport.MinDepth = 0.0f;
	viewport.MaxDepth = 1.0f;

	m_context->ClearRenderTargetView(m_renderTarget.Get(), &m_clearColor[0]);
	m_context->RSSetViewports(1, &viewport);
	m_context->OMSetRenderTargets(1, m_renderTarget.GetAddressOf(), nullptr);
	m_swapChain->Present(1, 0); // first arg -> 1 = regular vsync (sync every v-blank), 0 = unlimited fps (no synchronization)
}

bool Device::CreateSwapchainResources()
{
	ComPtr<ID3D11Texture2D> backBuffer{ nullptr };
	if (FAILED(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))))
	{
		log::Error("Failed to get Back Buffer from the Swap Chain");
		return false;
	}

	if (FAILED(m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, &m_renderTarget)))
	{
		log::Error("Failed to create Render Target View from Back Buffer");
		return false;
	}
	return true;
}

void Device::DestroySwapchainResources()
{
	m_renderTarget.Reset();
}
