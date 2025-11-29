#define NOMINMAX
#include "device.hpp"
#include "application.hpp"
#include "window.hpp"
#include "log.hpp"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

using namespace slate;

Device::Device(int width, int height)
	: m_width{ uint32_t(width) }
	, m_height{ uint32_t(height) }
{
}

Device::~Device()
{
	log::Info("Device destructor - flushing GPU...");
	Flush(m_commandQueue, m_fence, m_fenceValue, m_fenceEvent);

	log::Info("Device destructor - cleaning up resources...");
	for (int i = 0; i < m_numFrames; ++i)
	{
		m_commandAllocators[i].Reset();
		m_backBuffers[i].Reset();
	}

	m_commandList.Reset();
	m_commandQueue.Reset();
	m_rtvDescriptorHeap.Reset();
	m_fence.Reset();
	m_swapChain.Reset();
	m_dxgiFactory.Reset();
	m_device.Reset();

#if defined(_DEBUG)
	ComPtr<IDXGIDebug1> dxgiDebug;
	if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug))))
	{
		log::Info("Reporting live D3D12 objects...");
		log::ThrowIfFailed(dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_SUMMARY | DXGI_DEBUG_RLO_IGNORE_INTERNAL)));
	}
#endif
	log::Info("Device destroyed successfully");
}

bool Device::Initialize()
{
	log::Info("Initializing D3D12 Device...");
	EnableDebugLayer();
	log::Info("Getting GPU adapter...");
	ComPtr<IDXGIAdapter4> adapter = GetAdapter(false);
	if (!adapter)
	{
		log::Error("Failed to get GPU adapter");
		return false;
	}
	log::Info("Creating D3D12 device...");
	CreateDevice(adapter);
	log::Info("Creating command queue...");
	CreateCommandQueue(D3D12_COMMAND_LIST_TYPE_DIRECT);
	HWND hWnd = App.Window().GetHandle();
	if (!hWnd)
	{
		log::Error("Failed to get window handle");
		return false;
	}
	log::Info("Creating swap chain...");
	CreateSwapChain(hWnd, m_numFrames);
	m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
	log::Info("Current back buffer index: {}", m_currentBackBufferIndex);
	log::Info("Creating RTV descriptor heap...");
	CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE_RTV, m_numFrames);
	m_rtvDescriptorSize = m_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	log::Info("RTV descriptor size: {}", m_rtvDescriptorSize);
	log::Info("Updating render target views...");
	UpdateRenderTargetViews();
	log::Info("Creating command allocators...");
	for (int i = 0; i < m_numFrames; ++i)
	{
		m_commandAllocators[i] = CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT);
		log::Info("Command allocator {} created", i);
	}
	log::Info("Creating command list...");
	m_commandList = CreateCommandList(m_commandAllocators[m_currentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
	log::Info("Creating fence...");
	m_fence = CreateFence();
	log::Info("Creating fence event handle...");
	m_fenceEvent = CreateEventHandle();
	log::Info("D3D12 Device initialized successfully!");
	return true;
}

// TODO: implement later
bool Device::OnResizeEvent()
{

	return true;
}

void Device::Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();;
	auto deltaTime = t1 - t0;
	t0 = t1;
	elapsedSeconds += deltaTime.count() * 1e-9;
	if (elapsedSeconds > 1.0)
	{
		frameCounter = 0;
		elapsedSeconds = 0.0;
	}
}

void Device::Render()
{
	auto commandAllocator = m_commandAllocators[m_currentBackBufferIndex];
	auto backBuffer = m_backBuffers[m_currentBackBufferIndex];
	commandAllocator->Reset();
	m_commandList->Reset(commandAllocator.Get(), nullptr);
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
		m_commandList->ResourceBarrier(1, &barrier);
		CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(m_rtvDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), m_currentBackBufferIndex, m_rtvDescriptorSize);
		m_commandList->ClearRenderTargetView(rtv, &m_clearColor[0], 0, nullptr);
	}
	// Present
	{
		CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
		m_commandList->ResourceBarrier(1, &barrier);
		log::ThrowIfFailed(m_commandList->Close());
		ID3D12CommandList* const commandLists[] = { m_commandList.Get() };
		m_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);
		UINT syncInterval = m_vsync ? 1 : 0;
		UINT presentFlags = m_tearingSupported && !m_vsync ? DXGI_PRESENT_ALLOW_TEARING : 0;
		log::ThrowIfFailed(m_swapChain->Present(syncInterval, presentFlags));
		m_frameFenceValues[m_currentBackBufferIndex] = Signal(m_fence, m_fenceValue);
		m_currentBackBufferIndex = m_swapChain->GetCurrentBackBufferIndex();
		WaitForFenceValue(m_fence, m_frameFenceValues[m_currentBackBufferIndex], m_fenceEvent, std::chrono::milliseconds::max());
	}
}

void Device::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	log::ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&m_device)));

	// enable debug messages in debug mode
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if (SUCCEEDED(m_device.As(&pInfoQueue)))
	{
		log::Info("Debug layer enabled - configuring message filters...");

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
			// Since I think this bug will never be fixed in the debugger, it's best to just ignore this warning.
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE, };
		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;
		log::ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
		log::Info("Debug message filters configured");
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
	log::Info("Command queue created successfully");
}

void Device::CreateSwapChain(HWND hWnd, uint32_t bufferCount)
{
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	log::ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
	log::Info("DXGI Factory created");

	DXGI_SWAP_CHAIN_DESC1 scd = {};
	scd.Width = m_width;
	scd.Height = m_height;
	scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.Stereo = FALSE;
	scd.SampleDesc = { 1, 0 }; // This member is valid only with bit-block transfer (bitblt) model swap chains. When using flip model swap chain, this member must be specified as {1, 0}.
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // can also be used for shader input
	scd.BufferCount = bufferCount;
	scd.Scaling = DXGI_SCALING_STRETCH;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	m_tearingSupported = CheckForTearingSupport();
	scd.Flags = m_tearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	log::Info("Tearing support: {}", m_tearingSupported ? "enabled" : "disabled");

	ComPtr<IDXGISwapChain1> swapChain1;
	log::ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(m_commandQueue.Get(), hWnd, &scd, nullptr, nullptr, &swapChain1));
	log::ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hWnd, DXGI_MWA_NO_ALT_ENTER)); // disable alt + enter
	log::ThrowIfFailed(swapChain1.As(&m_swapChain));
	log::Info("Swap chain created successfully with {} buffers", bufferCount);
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

	log::ThrowIfFailed(m_device->CreateCommandList(0, type, allocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));

	log::ThrowIfFailed(commandList->Close());

	return commandList;
}

// fence = interface for gpu/cpu synchronization
// used to perform synchronization on either the CPU or GPU
// for each command queue use a fence
ComPtr<ID3D12Fence> Device::CreateFence()
{
	ComPtr<ID3D12Fence> fence;

	log::ThrowIfFailed(m_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

//  An OS event handle is used to block the CPU thread until the fence has been signaled. 
// The CreateEventHandle function described next is used to create the OS event.
// TODO: move to window
HANDLE Device::CreateEventHandle()
{
	HANDLE fenceEvent;

	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent && "Failed to create fence event");

	return fenceEvent;
}

// Signal the fence from the GPU, the fence is not signaled immediately but it is only signaled once GPU command queue has reached that point during execution
// Any commands that have been queued before the signal method was invoked 
uint64_t Device::Signal(ComPtr<ID3D12Fence> fence, uint64_t& fenceValue)
{
	uint64_t fenceValueForSignal = ++fenceValue;
	log::ThrowIfFailed(m_commandQueue->Signal(fence.Get(), fenceValueForSignal));

	return fenceValueForSignal;
}

// function for when CPU thread will need to stall for the GPU queue to finish executing commands that write to resources being reused
// e.g. before reusing a swapchain's backbuffer resource, any commands that are using that resource as a render target must be complete before that back buffer resource can be reused.
// any resources that are never used as a writeable target (for example material textures) do not need to be double buffered and do not require stalling the CPU thread before being reused
// as read-only resources in a shader.
// Writeable resources such as render targets do need to be synchronized to protect the resource from being modified by multiple queues at the same time
// The WaitForFenceValue is used to stall the CPU thread if the fence has not yet reached (been signaled with) a specific value. 
void Device::WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent, std::chrono::milliseconds duration)
{
	duration = std::chrono::milliseconds::max();

	if (fence->GetCompletedValue() < fenceValue)
	{
		log::ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

// flush is used to ensure that any commands previously executed on the GPU have finished executing before the CPU thread is allowed to continue processing. 
// Useful for ensuring any backbuffer resources being referenced by a command that is currently in-flight on the GPU have finished executing before being resized
// Advice: Flush the GPU Command Queue before releasing any resources that might be referenced by a command list that is currently in-flight on the command queue (e.g. before closing app)
void Device::Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t& fenceValue, HANDLE fenceEvent)
{
	uint64_t fenceValueForSignal = Signal(fence, fenceValue);
	WaitForFenceValue(fence, fenceValueForSignal, fenceEvent, std::chrono::milliseconds::max());
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
