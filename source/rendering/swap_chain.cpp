#include "pch.hpp"
#include "rendering/swap_chain.hpp"
#include "renderer.hpp"

using namespace slate;

void SwapChain::Initialize(HWND hWnd, u32 width, u32 height, u32 bufferCount)
{
	m_Width = width;
	m_Height = height;
	m_BufferCount = bufferCount;

	m_TearingSupported = CheckForTearingSupport();
	log::Info( "Tearing support: {}", m_TearingSupported ? "enabled" : "disabled" );

	auto commandQueue = App.Renderer().D3D12CommandQueue();

	CreateSwapChain( commandQueue, hWnd );
	CreateBackBuffers();

	log::Info( "SwapChain initialized: {}x{}, {} buffers", width, height, bufferCount );
}

void SwapChain::Present(bool vSync)
{
	UINT syncInterval = vSync ? 1 : 0;
	UINT presentFlags = ( m_TearingSupported && !vSync ) ? DXGI_PRESENT_ALLOW_TEARING : 0;

	log::ThrowIfFailed( m_SwapChain->Present( syncInterval, presentFlags ) );
}

void SwapChain::Resize(u32 width, u32 height)
{
	if ( width == m_Width && height == m_Height )
		return;

	log::Info( "Resizing swap chain: {}x{} -> {}x{}", m_Width, m_Height, width, height );

	m_Width = width;
	m_Height = height;

	// need to release back buffers before resizing
	ReleaseBackBuffers();

	DXGI_SWAP_CHAIN_DESC desc = {};
	m_SwapChain->GetDesc( &desc );

	log::ThrowIfFailed(
		m_SwapChain->ResizeBuffers(
			m_BufferCount,
			m_Width,
			m_Height,
			m_Format,
			desc.Flags
		)
	);

	// recreate back buffers
	CreateBackBuffers();

	log::Info( "Swap chain resized successfully" );
}

u32 SwapChain::GetCurrentBackBufferIndex() const
{
	return m_SwapChain->GetCurrentBackBufferIndex();
}

ComPtr<ID3D12Resource> SwapChain::GetCurrentBackBuffer() const
{
	return m_BackBuffers[ GetCurrentBackBufferIndex() ];
}

ComPtr<ID3D12Resource> SwapChain::GetBackBuffer(u32 index) const
{
	log::Assert(
		index < m_BufferCount, "Back buffer index {} out of range (max: {})",
		index, m_BufferCount
	);
	return m_BackBuffers[ index ];
}

void SwapChain::CreateSwapChain(ComPtr<ID3D12CommandQueue> commandQueue, HWND hWnd)
{
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	log::ThrowIfFailed(
		CreateDXGIFactory2(
			createFactoryFlags, IID_PPV_ARGS( &dxgiFactory4 )
		)
	);
	
	log::Info( "DXGI Factory created" );

	DXGI_SWAP_CHAIN_DESC1 scd = {};
	scd.Width = m_Width;
	scd.Height = m_Height;
	scd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	scd.Stereo = FALSE;
	scd.SampleDesc = { 1, 0 }; // This member is valid only with bit-block transfer (bitblt) model swap chains. When using flip model swap chain, this member must be specified as {1, 0}.
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // can also be used for shader input
	scd.BufferCount = m_BufferCount;
	scd.Scaling = DXGI_SCALING_STRETCH;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;

	scd.Flags = m_TearingSupported ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	log::Info( "Tearing support: {}", m_TearingSupported ? "enabled" : "disabled" );

	ComPtr<IDXGISwapChain1> swapChain1;
	log::ThrowIfFailed(
		dxgiFactory4->CreateSwapChainForHwnd(
			commandQueue.Get(), hWnd, &scd, nullptr, nullptr, &swapChain1
		)
	);

	log::ThrowIfFailed(
		dxgiFactory4->MakeWindowAssociation(
			hWnd, DXGI_MWA_NO_ALT_ENTER // disable alt + enter
		)
	); 

	log::ThrowIfFailed( swapChain1.As( &m_SwapChain ) );
	log::Info( "Swap chain created successfully with {} buffers", m_BufferCount );
}

void SwapChain::CreateBackBuffers()
{
	m_BackBuffers.resize( m_BufferCount );

	for ( u32 i{ 0u }; i < m_BufferCount; ++i)
	{
		log::ThrowIfFailed(
			m_SwapChain->GetBuffer(
				i, IID_PPV_ARGS( &m_BackBuffers[ i ] )
			)
		);
	}

	log::Info( "Created {} back buffers", m_BufferCount );
}

bool SwapChain::CheckForTearingSupport()
{
	BOOL allowTearing = FALSE;
	ComPtr<IDXGIFactory4> factory4;
	if ( SUCCEEDED( CreateDXGIFactory1( IID_PPV_ARGS( &factory4 ) ) ) )
	{
		ComPtr<IDXGIFactory5> factory5; // query for dxgi 1.5 interface
		if ( SUCCEEDED( factory4.As( &factory5 ) ) )
		{
			if (FAILED(
				factory5->CheckFeatureSupport(
					DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof( allowTearing ) ) ) )
			{
				allowTearing = FALSE;
			}
		}
	}

	return allowTearing == TRUE;
}

void SwapChain::ReleaseBackBuffers()
{
	for ( auto& buffer : m_BackBuffers )
	{
		buffer.Reset();
	}

	m_BackBuffers.clear();
}
