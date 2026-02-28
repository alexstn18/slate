#include "pch.hpp"
#include "rendering/device.hpp"
#include "window.hpp"

#if defined(_DEBUG)
#include <dxgidebug.h>
#endif

#include <d3dcompiler.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

using namespace slate;

Device::Device()
{
}

Device::~Device()
{
}

void Device::CreateDevice(ComPtr<IDXGIAdapter4> adapter)
{
	log::ThrowIfFailed(
		D3D12CreateDevice(
			adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS( &m_Device )
		)
	);

	// enable debug messages in debug mode
#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if ( SUCCEEDED( m_Device.As( &pInfoQueue ) ) ) {
		log::Info( "Debug layer enabled - configuring message filters..." );

		// SetBreakOnSeverity sets a message severity level to break on with a debugger
		// when a messages with that severity passes through the storage filter
		pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE ); // memory corruption
		pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_ERROR, TRUE );
		pInfoQueue->SetBreakOnSeverity( D3D12_MESSAGE_SEVERITY_WARNING, TRUE );

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
		NewFilter.DenyList.NumSeverities = _countof( Severities );
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof( DenyIds );
		NewFilter.DenyList.pIDList = DenyIds;
		log::ThrowIfFailed( pInfoQueue->PushStorageFilter( &NewFilter ) );
		log::Info( "Debug message filters configured" );
	}
#endif
	log::Info( "D3D12 Device created successfully!" );
}
