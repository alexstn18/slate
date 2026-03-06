#include "pch.hpp"

#include "window.hpp"

#include "rendering/adapter.hpp"
#include "rendering/device.hpp"
#include "rendering/command_queue.hpp"
#include "rendering/swap_chain.hpp"
#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"
#include "rendering/render_target.hpp"
#include "rendering/root_signature.hpp"
#include "rendering/pipeline_state_object.hpp"
#include "rendering/vertex_buffer.hpp"
#include "rendering/index_buffer.hpp"
#include "rendering/constant_buffer.hpp"
#include "rendering/structured_buffer.hpp"
#include "rendering/resource_state_tracker.hpp"

#include "rendering/model.hpp"
#include "rendering/mesh.hpp"
#include "rendering/texture.hpp"

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <d3dcompiler.h>

#include "interface.hpp"

using namespace slate;

Renderer::Renderer(u32 width, u32 height)
	: m_Width{ width }
	, m_Height{ height }
{
    m_Adapter = std::make_unique<Adapter>();
    m_Device = std::make_unique<Device>();
    m_CommandQueue = std::make_unique<CommandQueue>();
    m_SwapChain = std::make_unique<SwapChain>();
    m_DSVDescriptorHeap = std::make_unique<DescriptorHeap>();
    m_RTVDescriptorHeap = std::make_unique<DescriptorHeap>();
    m_SRVDescriptorHeap = std::make_unique<DescriptorHeap>();
    m_RenderTarget = std::make_unique<RenderTarget>();
    m_RootSignature = std::make_unique<RootSignature>();
    m_PipelineState = std::make_unique<PipelineStateObject>();
    m_Interface = std::make_unique<Interface>();
}

Renderer::~Renderer()
{
    // Flush the command queue.
    m_CommandQueue->Flush();

    ClearTextureCache();
    m_CommandList.reset();
    m_Model.reset();
    m_LightBuffer.reset();
    m_CommandList.reset();

    if (m_DepthStencilAllocation) {
        m_DepthStencilAllocation->Release();
        m_DepthStencilAllocation = nullptr;
    }
    m_DepthStencilBuffer.Reset();

    D3D12MA::TotalStatistics stats = {};
    m_Allocator->CalculateStatistics(&stats);

    log::Info("D3D12MA: {} allocations still alive ({} bytes)",
        stats.Total.Stats.AllocationCount,
        stats.Total.Stats.AllocationBytes);

    if (m_Allocator) {
        m_Allocator->Release();
        m_Allocator = nullptr;
    }
}

bool Renderer::Initialize()
{
	HWND hWnd = App.Window().GetHandle();

	EnableDebugLayer();
    m_Adapter->Initialize( false );  // Adapter created here
    m_Device->CreateDevice( m_Adapter->GetAdapter() );  // Device created here
	auto device = m_Device->GetDevice();

    D3D12MA::ALLOCATOR_DESC allocatorDesc = {};
    allocatorDesc.pDevice = m_Device->GetDevice().Get();
    allocatorDesc.pAdapter = m_Adapter->GetAdapter().Get();
    allocatorDesc.Flags = D3D12MA_RECOMMENDED_ALLOCATOR_FLAGS;

    log::ThrowIfFailed( D3D12MA::CreateAllocator( &allocatorDesc, &m_Allocator ) );

	m_CommandQueue->Initialize();
	m_SwapChain->Initialize( hWnd, m_Width, m_Height, m_NumBuffers );
	m_DSVDescriptorHeap->Initialize(
        HeapType::DSV, 
        device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_DSV )
    );
	m_RTVDescriptorHeap->Initialize(
        HeapType::RTV, 
        device->GetDescriptorHandleIncrementSize( D3D12_DESCRIPTOR_HEAP_TYPE_RTV )
    );
    static constexpr u32 MAX_TEXTURES = 1024u;
	m_SRVDescriptorHeap->Initialize( HeapType::SRV, MAX_TEXTURES );
	InitializeCommandAllocators();
    m_CommandList = std::make_shared<CommandList>();
    m_CommandList->Initialize();
	UpdateRenderTargetViews();
	CreateDepthStencil();

	m_Viewport = CD3DX12_VIEWPORT( 0.0f, 0.0f, float( m_Width ), float( m_Height ) );
	m_ScissorRect = CD3DX12_RECT( 0, 0, LONG( m_Width ), LONG( m_Height ) );

    log::Info( "Viewport: {}x{}", m_Viewport.Width, m_Viewport.Height );
    log::Info( "Scissor: {}x{}", m_ScissorRect.right, m_ScissorRect.bottom );

    m_Model = Model::Load(
        "assets/models/DamagedHelmet.glb", *m_CommandList, *m_CommandQueue, m_CommandAllocators[ 0 ]
    );
	CreateRootSignature();
	CompileShaders();

	// Setup render target
	auto rtv = m_RTVDescriptorHeap->GetCPUHandle( 0u );
	auto dsv = m_DSVDescriptorHeap->GetCPUHandle( 0u );
	m_RenderTarget->SetRenderTargetView( rtv );
	m_RenderTarget->SetDepthStencilView( dsv );

    m_Interface->Initialize();

    Light light;

    light.Color = glm::vec3(1.0f, 0.95f, 0.8f);
    light.Position = glm::vec3(3.0f, 3.0f, 5.0f);
    light.Intensity = 1.0f;

    m_Lights.push_back(light);
    m_CommandList->Reset(m_CommandAllocators[0]);
    m_LightBuffer = StructuredBuffer::Create(u32(m_Lights.size()), sizeof(Light), m_Lights.data());
    m_CommandList->Close();
    u64 fence = m_CommandQueue->ExecuteCommandLists({ m_CommandList->Get().Get() });
    m_CommandQueue->WaitForFenceValue(fence);
    FlushUploads();

	return true;
}

void Renderer::Shutdown()
{
    m_Interface->Shutdown();
}

void Renderer::Update()
{
	static uint64_t frameCounter = 0ull;
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

	if ( elapsedSeconds > 1.0 ) {
		frameCounter = 0;
		elapsedSeconds -= 1.0;
	}

	float angle = float( totalTime * 45.0f );

    const glm::mat4& projection = m_Camera.GetProjection( 
        float( m_Width ), float( m_Height ) 
    );
    const glm::mat4& view = m_Camera.GetView();

    m_Model->Update( projection * view, m_Camera.position );

    m_Interface->NewFrame();
    m_Interface->Update( float( deltaSeconds ) );
}

void Renderer::Render()
{
    u32 frameIndex = m_SwapChain->GetCurrentBackBufferIndex();
    auto backBuffer = m_SwapChain->GetBackBuffer( frameIndex );
    auto rtv = m_RTVDescriptorHeap->GetCPUHandle( frameIndex );
    m_RenderTarget->SetRenderTargetView( rtv );

    m_CommandList->Reset( m_CommandAllocators[ frameIndex ] );

    auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_PRESENT,
        D3D12_RESOURCE_STATE_RENDER_TARGET
    );
    m_CommandList->Get()->ResourceBarrier( 1u, &barrier );

    m_CommandList->SetRenderTarget( *m_RenderTarget );
    m_CommandList->ClearRenderTargetView( rtv, &m_ClearColor[ 0 ] );
    auto dsv = m_DSVDescriptorHeap->GetCPUHandle( 0u );
    m_CommandList->ClearDepthStencilView( dsv, D3D12_CLEAR_FLAG_DEPTH, 1.0f, 0u );

    m_CommandList->SetPipelineState(
        std::shared_ptr<PipelineStateObject>( m_PipelineState.get(), [](auto*) {} )
    );
    m_CommandList->SetGraphicsRootSignature(
        std::shared_ptr<RootSignature>( m_RootSignature.get(), [](auto*) {} )
    );

    ID3D12DescriptorHeap* heaps[] = { m_SRVDescriptorHeap->Get().Get() };
    m_CommandList->Get()->SetDescriptorHeaps( 1u, heaps );

    m_CommandList->SetViewport( m_Viewport );
    m_CommandList->SetScissorRect( m_ScissorRect );
    m_CommandList->SetPrimitiveTopology( D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // root index 0: model CBV (b0)
    m_CommandList->Get()->SetGraphicsRootDescriptorTable(
        0u, m_Model->GetConstantBuffer()->GetGPUHandle()
    );

    // root index 2: light structured buffer (t4)
    m_CommandList->Get()->SetGraphicsRootDescriptorTable(
        2u, m_LightBuffer->GetGPUSRVHandle()
    );

    for ( const auto& mesh : m_Model->GetMeshes() ) {
        const auto& material = mesh->GetMaterial();

        // root index 1: textures t0-t3
        if ( material && material->Albedo ) {
            m_CommandList->Get()->SetGraphicsRootDescriptorTable(
                1u, material->Albedo->GetGPUHandle()
            );
        }

        m_CommandList->SetVertexBuffer( 0u, *mesh->GetVertexBuffer() );
        m_CommandList->SetIndexBuffer( *mesh->GetIndexBuffer() );
        m_CommandList->DrawIndexed( u32( mesh->GetIndexCount() ), 1u, 0u, 0u, 0u );
    }

    m_Interface->Render();

    barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        backBuffer.Get(),
        D3D12_RESOURCE_STATE_RENDER_TARGET,
        D3D12_RESOURCE_STATE_PRESENT
    );
    m_CommandList->Get()->ResourceBarrier( 1, &barrier );

    m_CommandList->Close();
    u64 fenceValue = m_CommandQueue->ExecuteCommandLists(
        { m_CommandList->Get().Get() }
    );
    m_SwapChain->Present( true );
    m_CommandQueue->WaitForFenceValue( fenceValue );
}

void Renderer::TrackUpload(ComPtr<ID3D12Resource> resource, 
    D3D12MA::Allocation* allocation)
{
    m_PendingUploads.push_back( { std::move(resource), allocation } );
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
	log::ThrowIfFailed( ::D3D12GetDebugInterface( IID_PPV_ARGS( &debugInterface ) ) );
	debugInterface->EnableDebugLayer();
#endif
}

void Renderer::InitializeCommandAllocators()
{
	m_CommandAllocators.resize( m_NumBuffers );
	for ( u32 i{ 0u }; i < m_NumBuffers; ++i ) {
		log::ThrowIfFailed(
            m_Device->GetDevice()->CreateCommandAllocator(
                    D3D12_COMMAND_LIST_TYPE_DIRECT, 
                    IID_PPV_ARGS( &m_CommandAllocators[ i ] 
                )
            )
        );
	}
}

void Renderer::UpdateRenderTargetViews()
{
    auto device = m_Device->GetDevice();

    for ( u32 i{ 0u }; i < m_NumBuffers; ++i )
    {
        ComPtr<ID3D12Resource> backBuffer = m_SwapChain->GetBackBuffer( i );
        auto rtv = m_RTVDescriptorHeap->GetCPUHandle( i );
        device->CreateRenderTargetView( backBuffer.Get(), nullptr, rtv );

        ResourceStateTracker::AddGlobalResourceState(
            backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT
        );
    }
}

void Renderer::CreateDepthStencil()
{
    auto device = m_Device->GetDevice();

    D3D12_CLEAR_VALUE depthOptimizedClearValue = {};
    depthOptimizedClearValue.Format = DXGI_FORMAT_D32_FLOAT;
    depthOptimizedClearValue.DepthStencil.Depth = 1.0f;
    depthOptimizedClearValue.DepthStencil.Stencil = 0;

    CD3DX12_RESOURCE_DESC depthDesc = CD3DX12_RESOURCE_DESC::Tex2D(
        DXGI_FORMAT_D32_FLOAT, m_Width, m_Height,
        1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL
    );

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    log::ThrowIfFailed(
        m_Allocator->CreateResource(
            &allocDesc,
            &depthDesc,
            D3D12_RESOURCE_STATE_DEPTH_WRITE,
            &depthOptimizedClearValue,
            &m_DepthStencilAllocation,
            IID_PPV_ARGS( &m_DepthStencilBuffer )
        )
    );

    D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
    dsv.Format = DXGI_FORMAT_D32_FLOAT;
    dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
    dsv.Flags = D3D12_DSV_FLAG_NONE;

    auto dsvHandle = m_DSVDescriptorHeap->GetCPUHandle( 0u );
    device->CreateDepthStencilView( m_DepthStencilBuffer.Get(), &dsv, dsvHandle );

    ResourceStateTracker::AddGlobalResourceState(
        m_DepthStencilBuffer.Get(), D3D12_RESOURCE_STATE_DEPTH_WRITE
    );
}

void Renderer::CreateRootSignature()
{
    m_RootSignature
        ->AddDescriptorTable().AddCBVs(0u, 1u)   // b0 cbv
        .AddDescriptorTable().AddSRVs(0u, 4u)    // t0-t3 textures  
        .AddDescriptorTable().AddSRVs(4u, 1u)    // t4 lights
        .AddStaticSampler(0u);
    m_RootSignature->Initialize();
}

void Renderer::CompileShaders()
{
    auto device = m_Device->GetDevice();

    ComPtr<ID3DBlob> vertexShader;
    ComPtr<ID3DBlob> pixelShader;

    log::ThrowIfFailed( D3DCompileFromFile( L"assets/shaders/phong.hlsl", nullptr, nullptr,
        "VSMain", "vs_5_0", 0, 0, &vertexShader, nullptr ) );
    log::ThrowIfFailed( D3DCompileFromFile( L"assets/shaders/phong.hlsl", nullptr, nullptr,
        "PSMain", "ps_5_0", 0, 0, &pixelShader, nullptr ) );

    D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
        { "POSITION",  0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,  D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "NORMAL",    0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TEXCOORD",  0, DXGI_FORMAT_R32G32_FLOAT,    0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "TANGENT",   0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 32, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
        { "BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 44, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
    };

    D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};
    psoDesc.InputLayout = { inputLayout, _countof( inputLayout ) };
    psoDesc.pRootSignature = m_RootSignature->Get().Get();
    psoDesc.VS = CD3DX12_SHADER_BYTECODE( vertexShader.Get() );
    psoDesc.PS = CD3DX12_SHADER_BYTECODE( pixelShader.Get() );
    psoDesc.RasterizerState = CD3DX12_RASTERIZER_DESC( D3D12_DEFAULT );
    psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
    psoDesc.BlendState = CD3DX12_BLEND_DESC( D3D12_DEFAULT );
    psoDesc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC( D3D12_DEFAULT );
    psoDesc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
    psoDesc.SampleMask = UINT_MAX;
    psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
    psoDesc.NumRenderTargets = 1u;
    psoDesc.RTVFormats[ 0 ] = DXGI_FORMAT_R8G8B8A8_UNORM;
    psoDesc.SampleDesc.Count = 1u;

    m_PipelineState->InitializeAsGraphicsPSO( psoDesc );
}

void Renderer::FlushUploads()
{
    for ( auto& upload : m_PendingUploads )
    {
        if ( upload.Allocation )
        {
            upload.Allocation->Release();
            upload.Allocation = nullptr;
        }
    }

    m_PendingUploads.clear();
}