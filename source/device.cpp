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
	//m_context->Flush();
	//DestroySwapchainResources();
	//m_swapChain.Reset();
	//m_dxgiFactory.Reset();
	//m_context.Reset();
	//m_device.Reset();
}

bool Device::Initialize()
{
	return true;
}

bool Device::OnResizeEvent()
{

	return true;
}

void Device::Render()
{

}

bool Device::CreateSwapchainResources()
{

	return true;
}

void Device::DestroySwapchainResources()
{
	
}

void Device::EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface{ nullptr };
	log::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface));
	debugInterface->EnableDebugLayer();
#endif
}

// DXGIFactory is used to query for hardware adapters.
ComPtr<IDXGIAdapter4> Device::GetAdapter(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory{ nullptr };
	UINT createFactoryFlags{ 0 };
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	log::ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if (useWarp)
	{
		log::ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		log::ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4)); // casting ComPtr uses .As() function
	}
	else
	{
		SIZE_T maxDedicatedVideoMemory{ 0 };
		// Enumerate the available GPU adapters in the system. Returns DXGI_ERROR_NOT_FOUND if the adapter index
		// is greater than or equal to the number of available adapters.
		for (UINT i{ 0 }; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i)
		{
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory)
			{
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				log::ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));
			}
		}
	}

	return dxgiAdapter4;
}
