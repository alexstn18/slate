#include "device.hpp"
#include "application.hpp"
#include "window.hpp"
#include "log.hpp"

using namespace slate;

Device::Device(int width, int height)
	: m_width{ width }
	, m_height{ height }
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

void Device::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	log::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// enable debug messages in debug mode
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_device.As(&pInfoQueue)))
	{
		// SetBreakOnSeverity sets a message severity level to break on with a debugger
		// when a messages with that severity passes through the storage filter
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE); // memory corruption
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY Severities[] = { D3D12_MESSAGE_SEVERITY_INFO };
		D3D12_MESSAGE_ID DenyIds[] = {
			// This warning occurs when a render target is cleared using a clear color that is not the optimized clear color specified during resource creation. 
			// If you want to clear a render target using an arbitrary clear color, you should disable this warning.
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			// These warnings occur when a frame is captured using the graphics debugger integrated in Visual Studio. 
			// Since I think this bug will never be fixed in the debugger, it’s best to just ignore this warning.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE, 
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE, };
		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;
		log::ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
}

void Device::CreateCommandQueue(D3D12_COMMAND_LIST_TYPE type)
{
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	log::ThrowIfFailed(m_device->CreateCommandQueue(&desc, IID_PPV_ARGS(&m_commandQueue)));
}

void Device::CreateSwapChain(HWND hWnd, uint32_t bufferCount)
{
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	log::ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));

	DXGI_SWAP_CHAIN_DESC1 scd = {};
	scd.Width = m_width;
	scd.Height = m_height;
	scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.Stereo = FALSE;
	scd.SampleDesc = {1, 0}; // This member is valid only with bit-block transfer (bitblt) model swap chains. When using flip model swap chain, this member must be specified as {1, 0}.
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // can also be used for shader input
	scd.BufferCount = bufferCount;
	scd.Scaling = DXGI_SCALING_STRETCH;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	scd.Flags = CheckForTearingSupport() ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	log::ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(m_commandQueue.Get(), hWnd, &scd, nullptr, nullptr, &swapChain1));
	log::ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)); // disable alt + enter
	log::ThrowIfFailed(swapChain1.As(&m_swapChain));
}

// descriptor heap = array of resource views
// in dx12 a descriptor heap is created before RTV, SRV, UAV or CBV
// RTV = Render Target View, SRV = Shader Resource View, UAV = Unordered Access View, CBV = Constant Buffer View
// CBV, SRV and UAV can be stored in the same descriptor heap but RTV and Sampler views require separate descriptor heaps

// current descriptor heap stores the RTV for the swapchain buffers
void Device::CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors)
{
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;

	log::ThrowIfFailed(m_device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_rtvDescriptorHeap)));
}

ComPtr<ID3D12CommandAllocator> Device::CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE type)
{
	// command list types:
	//D3D12_COMMAND_LIST_TYPE_DIRECT: Specifies a command buffer that the GPU can execute.A direct command list doesn’t inherit any GPU state.
	//D3D12_COMMAND_LIST_TYPE_BUNDLE : Specifies a command buffer that can be executed only directly via a direct command list.A bundle command list inherits all GPU state(except for the currently set pipeline state object and primitive topology).
	//D3D12_COMMAND_LIST_TYPE_COMPUTE : Specifies a command buffer for computing.
	//D3D12_COMMAND_LIST_TYPE_COPY : Specifies a command buffer for copying.

	ComPtr<ID3D12CommandAllocator> allocator;

	log::ThrowIfFailed(m_device->CreateCommandAllocator(type, IID_PPV_ARGS(&allocator)));

	return allocator;
}

ComPtr<ID3D12GraphicsCommandList> Device::CreateCommandList(ComPtr<ID3D12CommandAllocator> allocator, D3D12_COMMAND_LIST_TYPE type)
{
	ComPtr<ID3D12GraphicsCommandList> commandList;

	log::ThrowIfFailed(m_device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList));

	log::ThrowIfFailed(commandList->Close());

	return commandList;
}

// rtv describes the resource that receives the final color computed by the pixel/fragment shader stage
// for each back buffer of the swap chain, a single rtv is used to describe the resource
void Device::UpdateRenderTargetViews()
{
	auto rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for (int i = 0; i < m_numFrames; ++i)
	{
		ComPtr<ID3D12Resource> backBuffer;
		log::ThrowIfFailed(m_swapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		m_device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		m_backBuffers[i] = backBuffer;
		rtvHandle.Offset(rtvDescriptorSize);
	}
}

bool Device::CheckForTearingSupport()
{
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory4> factory4;
	if (SUCCEEDED(CreateDXGIFactory1(IID_PPV_ARGS(&factory4))))
	{
		ComPtr<IDXGIFactory5> factory5; // query for dxgi 1.5 interface
		if (SUCCEEDED(factory4.As(&factory5)))
		{
			if (FAILED(factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing))))
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
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
	log::ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
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
