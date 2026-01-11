#include "pch.hpp"

#include "window.hpp"

#include "rendering/adapter.hpp"
#include "device.hpp"
#include "rendering/command_queue.hpp"
#include "rendering/swap_chain.hpp"
#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"
#include "rendering/root_signature.hpp"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

using namespace slate;

Renderer::Renderer(u32 width, u32 height)
	: m_Width{width}
	, m_Height{height}
{
	m_Adapter = std::make_unique<Adapter>();
	m_Device = std::make_unique<Device>(m_Width, m_Height);
	m_CommandQueue = std::make_unique<CommandQueue>();
	m_SwapChain = std::make_unique<SwapChain>();
	m_DSVDescriptorHeap = std::make_unique<DescriptorHeap>();
	m_RTVDescriptorHeap = std::make_unique<DescriptorHeap>();
	m_CommandList = std::make_unique<CommandList>();
	m_RootSignature = std::make_unique<RootSignature>();
}

bool Renderer::Initialize()
{
	HWND hWnd = App.Window().GetHandle();

	EnableDebugLayer();
	m_Adapter->Initialize(false);
	m_Device->CreateDevice(m_Adapter->GetAdapter());
	auto device = m_Device->GetDevice();

	m_CommandQueue->Initialize();
	m_SwapChain->Initialize(hWnd, m_Width, m_Height, m_NumBuffers);
	m_DSVDescriptorHeap->Initialize(HeapType::DSV, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	m_RTVDescriptorHeap->Initialize(HeapType::RTV, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	m_CommandList->Initialize();
	m_RootSignature->Initialize();
	// compile shaders and create vertex buffer from here

	return true;
}

void Renderer::Update()
{
	static uint64_t frameCounter = 0;
	static double elapsedSeconds = 0.0;
	static double totalTime = 0.0;
	static std::chrono::high_resolution_clock clock;
	static auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1 - t0;
	t0 = t1;

	double deltaSeconds = deltaTime.count() * 1e-9;
	elapsedSeconds += deltaSeconds;
	totalTime += deltaSeconds;

	if (elapsedSeconds > 1.0)
	{
		frameCounter = 0;
		elapsedSeconds -= 1.0;
	}

	float angle = float(totalTime * 90.0f);

	glm::mat4 modelMatrix = glm::rotate(glm::mat4(1.0f), glm::radians(angle), glm::vec3(0.0f, 1.0f, 1.0f));

	glm::vec3 eyePos = glm::vec3(0.0f, 0.0f, -10.0f);
	glm::vec3 focusPoint = glm::vec3(0.0f, 0.0f, 0.0f);
	glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
	glm::mat4 view = glm::lookAtLH(eyePos, focusPoint, up);

	glm::mat4 projection = glm::perspectiveFovLH(glm::radians(45.0f), float(m_Width), float(m_Height), 0.1f, 100.0f);

	//m_mvpMatrix = projection * view * modelMatrix;
}

void Renderer::Render()
{
}

ComPtr<IDXGIAdapter4> Renderer::D3D12Adapter() const noexcept
{
	return m_Adapter->GetAdapter();
}

ComPtr<ID3D12Device2> Renderer::D3D12Device() const noexcept
{
	return m_Device->GetDevice();
}

ComPtr<ID3D12CommandQueue> Renderer::D3D12CommandQueue() const noexcept
{
	return m_CommandQueue->GetCommandQueue();
}

ComPtr<IDXGISwapChain4> Renderer::D3D12SwapChain() const noexcept
{
	return m_SwapChain->Get();
}

ComPtr<ID3D12GraphicsCommandList> Renderer::D3D12CommandList() const noexcept
{
	return m_CommandList->Get();
}

ComPtr<ID3D12RootSignature> Renderer::D3D12RootSignature() const noexcept
{
	return m_RootSignature->Get();
}

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface{ nullptr };
	log::ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}
