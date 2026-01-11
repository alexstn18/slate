#include "pch.hpp"

#include "window.hpp"

#include "rendering/adapter.hpp"
#include "device.hpp"
#include "rendering/command_queue.hpp"
#include "rendering/swap_chain.hpp"
#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"
#include "rendering/render_target.hpp"
#include "rendering/root_signature.hpp"
#include "rendering/pipeline_state_object.hpp"
#include "rendering/vertex_buffer.hpp"
#include "rendering/index_buffer.hpp"
#include "rendering/resource_state_tracker.hpp"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <d3dcompiler.h>

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
    m_RenderTarget = std::make_unique<RenderTarget>();
    m_RootSignature = std::make_unique<RootSignature>();
    m_PipelineState = std::make_unique<PipelineStateObject>();
}

bool Renderer::Initialize()
{
	HWND hWnd = App.Window().GetHandle();

	EnableDebugLayer();
    m_Adapter->Initialize(false);  // Adapter created here
    m_Device->CreateDevice(m_Adapter->GetAdapter());  // Device created here
	auto device = m_Device->GetDevice();

	m_CommandQueue->Initialize();
	m_SwapChain->Initialize(hWnd, m_Width, m_Height, m_NumBuffers);
	m_DSVDescriptorHeap->Initialize(HeapType::DSV, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV));
	m_RTVDescriptorHeap->Initialize(HeapType::RTV, device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV));
	InitializeCommandAllocators();
    m_CommandList = std::make_shared<CommandList>();
    m_CommandList->Initialize();
	UpdateRenderTargetViews();
	CreateDepthStencil();

	m_Viewport = CD3DX12_VIEWPORT(0.0f, 0.0f, float(m_Width), float(m_Height));
	m_ScissorRect = CD3DX12_RECT(0, 0, LONG(m_Width), LONG(m_Height));

	CreateRootSignature();
	CompileShaders();
	CreateVertexBuffer();

	// Setup render target
	auto rtv = m_RTVDescriptorHeap->GetCPUHandle(0);
	auto dsv = m_DSVDescriptorHeap->GetCPUHandle(0);
	m_RenderTarget->SetRenderTargetView(rtv);
	m_RenderTarget->SetDepthStencilView(dsv);

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

	m_MVPMatrix = projection * view * modelMatrix;
}

void Renderer::Render()
{
    u32 frameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    auto backBuffer = m_SwapChain->GetBackBuffer(frameIndex);

    auto rtv = m_RTVDescriptorHeap->GetCPUHandle(frameIndex);
    m_RenderTarget->SetRenderTargetView(rtv);

    m_CommandList->Reset(m_CommandAllocators[frameIndex]);

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_CommandList->Get()->ResourceBarrier(1, &barrier);

    m_CommandList->SetRenderTarget(*m_RenderTarget);
    m_CommandList->ClearRenderTargetView(rtv, &m_ClearColor[0]);
    auto dsv = m_DSVDescriptorHeap->GetCPUHandle(0);
    m_CommandList->ClearDepthStencilView(dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0);

    m_CommandList->SetPipelineState(std::shared_ptr<PipelineStateObject>(m_PipelineState.get(), [](auto*) {}));
    m_CommandList->SetGraphicsRootSignature(std::shared_ptr<RootSignature>(m_RootSignature.get(), [](auto*) {}));
    m_CommandList->SetGraphics32BitConstants(0, sizeof(glm::mat4) / 4, &m_MVPMatrix);
    m_CommandList->SetViewport(m_Viewport);
    m_CommandList->SetScissorRect(m_ScissorRect);
    m_CommandList->SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    m_CommandList->SetVertexBuffer(0, *m_VertexBuffer);
    m_CommandList->SetIndexBuffer(*m_IndexBuffer);
    m_CommandList->DrawIndexed(36, 1, 0, 0, 0);

    // Transition back to present
    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_CommandList->Get()->ResourceBarrier(1, &barrier);

    m_CommandList->Close();
    u64 fenceValue = m_CommandQueue->ExecuteCommandLists({ m_CommandList->Get().Get() });

    m_SwapChain->Present(true);
    m_CommandQueue->WaitForFenceValue(fenceValue);
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

void Renderer::EnableDebugLayer()
{
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface{ nullptr };
	log::ThrowIfFailed(::D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif
}

void Renderer::InitializeCommandAllocators()
{
	m_CommandAllocators.resize(m_NumBuffers);
	for (u32 i{ 0u }; i < m_NumBuffers; ++i) {
		log::ThrowIfFailed(m_Device->GetDevice()->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&m_CommandAllocators[i])));
	}
}

void Renderer::UpdateRenderTargetViews()
{
    auto device = m_Device->GetDevice();

    for (u32 i = 0; i < m_NumBuffers; ++i)
    {
        ComPtr<ID3D12Resource> backBuffer = m_SwapChain->GetBackBuffer(i);
        auto rtv = m_RTVDescriptorHeap->GetCPUHandle(i);
        device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtv);

        ResourceStateTracker::AddGlobalResourceState(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT);
    }
}

void Renderer::CreateDepthStencil()
{
    auto device = m_Device->GetDevice();

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    CD3DX12_HEAP_PROPERTIES depthHeapProps(D3D12_HEAP_TYPE_DEFAULT);
    CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, m_Width, m_Height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    log::ThrowIfFailed(device->CreateCommittedResource(
        &depthHeapProps,
        D3D12_HEAP_FLAG_NONE,
        &depthDesc,
        D3D12_RESOURCE_STATE_DEPTH_WRITE,
        &depthOptimizedClearValue,
        IID_PPV_ARGS(&m_DepthStencilBuffer)
    ));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    auto dsvHandle = m_DSVDescriptorHeap->GetCPUHandle(0);
    device->CreateDepthStencilView(m_DepthStencilBuffer.Get(), &dsv, dsvHandle);

    ResourceStateTracker::AddGlobalResourceState(m_DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE);
}

void Renderer::CreateRootSignature()
{
    m_RootSignature->AddRootConstants(0, sizeof(glm::mat4) / 4);
    m_RootSignature->Initialize();
}

void Renderer::CompileShaders()
{
    auto device = m_Device->GetDevice();

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    log::ThrowIfFailed(D3DCompileFromFile(L"shaders/cube.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr));
    log::ThrowIfFailed(D3DCompileFromFile(L"shaders/cube.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr));

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof(inputLayout) };
    psoDesc.pRootSignature = m_RootSignature->Get().Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE(vertexShader.Get());
    psoDesc.PS = CD3DX12_SHADER_BYTECODE(pixelShader.Get());
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1;
    psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1;

    m_PipelineState->InitializeAsGraphicsPSO(psoDesc);
}

void Renderer::CreateVertexBuffer()
{
    struct Vertex {
        glm::vec3 position;
        glm::vec4 color;
    };

    Vertex cubeVertices[] = {
        { glm::vec3(-1.0f, -1.0f,  1.0f), glm::vec4(1.0f, 0.0f, 0.0f, 1.0f) },
        { glm::vec3(1.0f, -1.0f,  1.0f), glm::vec4(0.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(1.0f,  1.0f,  1.0f), glm::vec4(0.0f, 0.0f, 1.0f, 1.0f) },
        { glm::vec3(-1.0f,  1.0f,  1.0f), glm::vec4(1.0f, 1.0f, 0.0f, 1.0f) },
        { glm::vec3(-1.0f, -1.0f, -1.0f), glm::vec4(1.0f, 0.0f, 1.0f, 1.0f) },
        { glm::vec3(1.0f, -1.0f, -1.0f), glm::vec4(0.0f, 1.0f, 1.0f, 1.0f) },
        { glm::vec3(1.0f,  1.0f, -1.0f), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f) },
        { glm::vec3(-1.0f,  1.0f, -1.0f), glm::vec4(0.5f, 0.5f, 0.5f, 1.0f) }
    };

    u32 cubeIndices[] = {
        0,1,2, 2,3,0,  5,4,7, 7,6,5,  4,0,3, 3,7,4,
        1,5,6, 6,2,1,  3,2,6, 6,7,3,  4,5,1, 1,0,4
    };

    m_CommandList->Reset(m_CommandAllocators[0]);

    m_VertexBuffer = std::make_unique<VertexBuffer>(8, sizeof(Vertex));
    m_IndexBuffer = std::make_unique<IndexBuffer>(36, DXGI_FORMAT_R32_UINT);

    m_CommandList->UploadBufferData(*m_VertexBuffer, cubeVertices, sizeof(cubeVertices));
    m_CommandList->UploadBufferData(*m_IndexBuffer, cubeIndices, sizeof(cubeIndices));
    m_CommandList->Close();
    u64 fenceValue = m_CommandQueue->ExecuteCommandLists({ m_CommandList->Get().Get() });

    // wait for upload to complete before rendering
    m_CommandQueue->WaitForFenceValue(fenceValue);
}