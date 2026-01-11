#include "pch.hpp"
#include "rendering/adapter.hpp"

using namespace slate;

void Adapter::Initialize(bool useWarp)
{
	ComPtr<IDXGIFactory4> dxgiFactory{ nullptr };
	UINT createFactoryFlags{ 0 };
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	log::ThrowIfFailed(::CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	
	ComPtr<IDXGIAdapter1> dxgiAdapter1{ nullptr };
	if (useWarp) {
		log::ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		log::ThrowIfFailed(dxgiAdapter1.As(&m_Adapter)); // casting ComPtr uses .As() function
	}
	else {
		SIZE_T maxDedicatedVideoMemory{ 0 };
		// Enumerate the available GPU adapters in the system. Returns DXGI_ERROR_NOT_FOUND if the adapter index
		// is greater than or equal to the number of available adapters.
		for (UINT i{ 0u }; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1) != DXGI_ERROR_NOT_FOUND; ++i) {
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1{};
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			m_GPUName = std::wstring(dxgiAdapterDesc1.Description);
			m_GPUMemoryInMB = static_cast<i32>(dxgiAdapterDesc1.DedicatedVideoMemory) / (1024 * 1024);
			log::Info("GPU: {}", GetGPUName());
			log::Info("VRAM: {} MB", m_GPUMemoryInMB);

			// Check to see if the adapter can create a D3D12 device without actually 
			// creating it. The adapter with the largest dedicated video memory
			// is favored.
			if ((dxgiAdapterDesc1.Flags & DXGI_ADAPTER_FLAG_SOFTWARE) == 0 &&
				SUCCEEDED(::D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr)) &&
				dxgiAdapterDesc1.DedicatedVideoMemory > maxDedicatedVideoMemory) {
				maxDedicatedVideoMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				log::ThrowIfFailed(dxgiAdapter1.As(&m_Adapter));
			}
		}
	}

	log::Info("DXGI Adapter successfully initialized");
}
