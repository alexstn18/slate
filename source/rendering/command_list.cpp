#include "pch.hpp"
#include "rendering/command_list.hpp"
#include "rendering/resource.hpp"
#include "rendering/buffer.hpp"
#include "rendering/upload_buffer.hpp"
#include "rendering/resource_state_tracker.hpp"
#include "rendering/dynamic_descriptor_heap.hpp"
#include "rendering/pipeline_state_object.hpp"
#include "rendering/root_signature.hpp"
#include "rendering/render_target.hpp"
#include "rendering/vertex_buffer.hpp"
#include "rendering/index_buffer.hpp"

using namespace slate;

class MakeUploadBuffer : public UploadBuffer
{
public:
	MakeUploadBuffer(size_t pageSize = _2MB)
		: UploadBuffer{ pageSize }
	{
	}

	virtual ~MakeUploadBuffer() {}
};

CommandList::CommandList()
{
	auto device = App.Renderer().D3D12Device();
	m_UploadBuffer = std::make_unique<MakeUploadBuffer>();
	m_ResourceStateTracker = std::make_unique<ResourceStateTracker>();

	for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i )
	{
		m_DynamicDescriptorHeap[i] =
			std::make_unique<DynamicDescriptorHeap>(
				static_cast<D3D12_DESCRIPTOR_HEAP_TYPE>( i )
			);
	}
}

CommandList::~CommandList()
{
}

void CommandList::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
	auto device = App.Renderer().D3D12Device();

	m_CommandListType = type;

	ComPtr<ID3D12CommandAllocator> tempAllocator{ nullptr };

	log::ThrowIfFailed(
		device->CreateCommandAllocator( 
			m_CommandListType, IID_PPV_ARGS( &tempAllocator ) 
		)
	);

	log::ThrowIfFailed(
		device->CreateCommandList(
			0u, m_CommandListType, tempAllocator.Get(), nullptr, 
			IID_PPV_ARGS( &m_CommandList ) 
		)
	);

	log::ThrowIfFailed( m_CommandList->Close() );
}

void CommandList::Reset(ComPtr<ID3D12CommandAllocator> allocator)
{
	assert( allocator );
	m_CommandAllocator = allocator;

	log::ThrowIfFailed( m_CommandAllocator->Reset() );
	log::ThrowIfFailed( m_CommandList->Reset( m_CommandAllocator.Get(), nullptr ) ); // nullptr = Pipeline State Object, optional

	m_ResourceStateTracker->Reset();
	m_UploadBuffer->Reset();

	ReleaseTrackedObjects();

	for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
		m_DynamicDescriptorHeap[i]->Reset();
	}

	m_RootSignature = nullptr;
    m_PipelineState = nullptr;
	m_ComputeCommandList = nullptr;
}

bool CommandList::Close(const std::shared_ptr<CommandList>& pendingCommandList)
{
	FlushResourceBarriers();
	
	log::ThrowIfFailed( m_CommandList->Close() );

	u32 numPendingBarriers{ 
		m_ResourceStateTracker->FlushPendingResourceBarriers( pendingCommandList ) 
	};

	m_ResourceStateTracker->CommitFinalResourceStates();

	return numPendingBarriers > 0;
}

void CommandList::Close()
{
	FlushResourceBarriers();
	log::ThrowIfFailed( m_CommandList->Close() );
}

void CommandList::TransitionBarrier(const std::shared_ptr<Resource>& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
	auto d3d12Resource = resource->D3D12Resource();
	if ( d3d12Resource ) {
		// The "before" state is not important
		// It will be resolved by the resource state tracker
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource
		);

		m_ResourceStateTracker->ResourceBarrier( barrier );
	}

	if ( flushBarriers ) {
		FlushResourceBarriers();
	}
}

void CommandList::TransitionBarrier(
	ComPtr<ID3D12Resource> resource, D3D12_RESOURCE_STATES stateAfter, 
	UINT subResource, bool flushBarriers)
{
	if ( resource ) {
		// The "before" state is not important. It will be resolved by the resource state tracker.
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(
			resource.Get(), D3D12_RESOURCE_STATE_COMMON, 
			stateAfter, subResource
		);

		m_ResourceStateTracker->ResourceBarrier( barrier );
	}

	if ( flushBarriers ) {
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
	auto d3d12Resource = resource.D3D12Resource();
	if ( d3d12Resource ) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::UAV( d3d12Resource.Get() );

		m_ResourceStateTracker->ResourceBarrier( barrier );

		if ( flushBarriers ) {
			FlushResourceBarriers();
		}
	}
}

void CommandList::AliasingBarrier(
	const Resource& beforeRes, const Resource& afterRes, bool flushBarriers)
{
	auto d3d12BeforeRes = beforeRes.D3D12Resource();
	auto d3d12AfterRes = afterRes.D3D12Resource();
	if ( d3d12BeforeRes && d3d12AfterRes ) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(
			d3d12BeforeRes.Get(), d3d12AfterRes.Get()
		);

		m_ResourceStateTracker->ResourceBarrier( barrier );

		if ( flushBarriers ) {
			FlushResourceBarriers();
		}
	}
}

void CommandList::FlushResourceBarriers()
{
	m_ResourceStateTracker->FlushResourceBarriers( shared_from_this() );
}

void CommandList::TrackObject(ComPtr<ID3D12Object> object)
{
	m_TrackedObjects.push_back( object );
}

void CommandList::ReleaseTrackedObjects()
{
	m_TrackedObjects.clear();
}

void CommandList::TrackResource(ComPtr<ID3D12Object> object)
{
	m_TrackedObjects.push_back( object );
}

void CommandList::TrackResource(const std::shared_ptr<Resource>& res)
{
	TrackObject( res->D3D12Resource() );
}

void CommandList::TrackResource(const Resource& res)
{
	TrackObject( res.D3D12Resource() );
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
	// Copy queues can only transition to/from COMMON state
	// Direct/Compute queues can use COPY_DEST state
	TransitionBarrier(
		dstRes.D3D12Resource(), m_CommandListType == D3D12_COMMAND_LIST_TYPE_COPY ?
		D3D12_RESOURCE_STATE_COMMON :
		D3D12_RESOURCE_STATE_COPY_DEST
	);

	TransitionBarrier(
		srcRes.D3D12Resource(), m_CommandListType == D3D12_COMMAND_LIST_TYPE_COPY ?
		D3D12_RESOURCE_STATE_COMMON :
		D3D12_RESOURCE_STATE_COPY_SOURCE
	);

	FlushResourceBarriers();

	m_CommandList->CopyResource(
		dstRes.D3D12Resource().Get(), srcRes.D3D12Resource().Get()
	);

	TrackResource( dstRes );
	TrackResource( srcRes );
}

void CommandList::ResolveSubResource(
	const std::shared_ptr<Resource>& dstRes, 
	const std::shared_ptr<Resource>& srcRes, 
	u32 dstSubResource, u32 srcSubResource)
{
	assert( dstRes && srcRes );

	TransitionBarrier( dstRes, D3D12_RESOURCE_STATE_RESOLVE_DEST, dstSubResource );
	TransitionBarrier( srcRes, D3D12_RESOURCE_STATE_RESOLVE_SOURCE, srcSubResource );

	FlushResourceBarriers();

	m_CommandList->ResolveSubresource(
		dstRes->D3D12Resource().Get(), dstSubResource,
		srcRes->D3D12Resource().Get(), srcSubResource,
		dstRes->GetResourceDesc().Format
	);

	TrackResource( srcRes );
	TrackResource( dstRes );
}

void CommandList::SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY topology)
{
	m_CommandList->IASetPrimitiveTopology( topology );
}

void CommandList::SetPipelineState(
	const std::shared_ptr<PipelineStateObject>& pipelineState)
{
	assert( pipelineState );

	auto d3d12PipelineStateObject = pipelineState->GetD3D12PipelineState().Get();
	if ( m_PipelineState != d3d12PipelineStateObject ) {
		m_PipelineState = d3d12PipelineStateObject;

		m_CommandList->SetPipelineState( d3d12PipelineStateObject );

		TrackResource( d3d12PipelineStateObject );
	}
}

void CommandList::SetGraphics32BitConstants(
	u32 rootParameterIndex, u32 numConstants, const void* constants)
{
	m_CommandList->SetGraphicsRoot32BitConstants(
		rootParameterIndex, numConstants, constants, 0u
	);
}

void CommandList::SetCompute32BitConstants(
	u32 rootParameterIndex, u32 numConstants, const void* constants)
{
	m_CommandList->SetComputeRoot32BitConstants(
		rootParameterIndex, numConstants, constants, 0u
	);
}

void CommandList::SetGraphicsRootSignature(
	const std::shared_ptr<RootSignature>& rootSignature)
{
	assert( rootSignature );

	auto d3d12RootSignature = rootSignature->Get().Get();
	if ( m_RootSignature != d3d12RootSignature ) {
		m_RootSignature = d3d12RootSignature;

		for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
			m_DynamicDescriptorHeap[i]->ParseRootSignature( *rootSignature );
		}

		m_CommandList->SetGraphicsRootSignature( m_RootSignature );

		TrackResource( m_RootSignature );
	}
}

void CommandList::SetComputeRootSignature(
	const std::shared_ptr<RootSignature>& rootSignature)
{
	assert( rootSignature );

	auto d3d12RootSignature = rootSignature->Get().Get();
	if ( m_RootSignature != d3d12RootSignature ) {
		m_RootSignature = d3d12RootSignature;

		for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
			m_DynamicDescriptorHeap[i]->ParseRootSignature( *rootSignature );
		}

		m_CommandList->SetComputeRootSignature( m_RootSignature );

		TrackResource( m_RootSignature );
	}
}

void CommandList::SetGraphicsDynamicConstantBuffer(
	u32 rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned
	auto heapAllocation{ 
		m_UploadBuffer->Allocate(
			sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT
		) 
	};

	memcpy( heapAllocation.CPU, bufferData, sizeInBytes );

	m_CommandList->SetGraphicsRootConstantBufferView(
		rootParameterIndex, heapAllocation.GPU
	);
}

void CommandList::SetShaderResourceView(
	u32 rootParameterIndex, u32 descriptorOffset, 
	const std::shared_ptr<Resource>& resource, 
	D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource, 
	UINT numSubResources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if ( numSubResources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES ) {
		for ( u32 i{ 0u }; i < numSubResources; ++i ) {
			TransitionBarrier( resource, stateAfter, firstSubResource + i );
		}
	}
	else {
		TransitionBarrier( resource, stateAfter );
	}

	m_DynamicDescriptorHeap[ D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ]->StageDescriptors(
		rootParameterIndex, descriptorOffset, 1, resource->GetShaderResourceView( srv )
	);

	TrackResource( resource );
}

void CommandList::Draw(
	u32 vertexCount, u32 instanceCount, 
	u32 startVertex, u32 startInstance)
{
	FlushResourceBarriers();

	for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
		m_DynamicDescriptorHeap[ i ]->CommitStagedDescriptorsForDraw( *this );
	}
	m_CommandList->DrawInstanced(
		vertexCount, instanceCount, startVertex, startInstance
	);
}

void CommandList::DrawIndexed(
	u32 indexCount, u32 instanceCount, 
	u32 startIndex, i32 baseVertex, 
	u32 startInstance)
{
	FlushResourceBarriers();

	for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
		m_DynamicDescriptorHeap[ i ]->CommitStagedDescriptorsForDraw( *this );
	}

	m_CommandList->DrawIndexedInstanced(
		indexCount, instanceCount, startIndex, baseVertex, startInstance
	);
}

void CommandList::SetViewport(const D3D12_VIEWPORT& viewport)
{
	SetViewports( { viewport } );
}

void CommandList::SetViewports(const std::vector<D3D12_VIEWPORT>& viewports)
{
	assert( viewports.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE );
	m_CommandList->RSSetViewports(
		static_cast<UINT>(viewports.size()), viewports.data()
	);
}

void CommandList::SetScissorRect(const D3D12_RECT& scissorRect)
{
	SetScissorRects( { scissorRect } );
}

void CommandList::SetScissorRects(const std::vector<D3D12_RECT>& scissorRects)
{
	assert( 
		scissorRects.size() < D3D12_VIEWPORT_AND_SCISSORRECT_OBJECT_COUNT_PER_PIPELINE
	);
	m_CommandList->RSSetScissorRects(
		static_cast<UINT>( scissorRects.size() ), scissorRects.data()
	);
}

void CommandList::SetRenderTarget(const RenderTarget& renderTarget)
{
	const auto& rtvs = renderTarget.GetRenderTargetViews();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = renderTarget.GetDepthStencilView();

	m_CommandList->OMSetRenderTargets(
		static_cast<UINT>( rtvs.size() ),
		rtvs.data(),
		FALSE,
		renderTarget.HasDepthStencil() ? &dsv : nullptr
	);
}

void CommandList::SetVertexBuffer(u32 slot, const VertexBuffer& vertexBuffer)
{
	auto vbv = vertexBuffer.GetVertexBufferView();
	m_CommandList->IASetVertexBuffers( slot, 1, &vbv );
	TrackResource( vertexBuffer );
}

void CommandList::SetIndexBuffer(const IndexBuffer& indexBuffer)
{
	auto ibv = indexBuffer.GetIndexBufferView();
	m_CommandList->IASetIndexBuffer( &ibv );
	TrackResource( indexBuffer );
}

void CommandList::Dispatch(u32 numGroupsX, u32 numGroupsY, u32 numGroupsZ)
{
	FlushResourceBarriers();

	for ( i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i ) {
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDispatch( *this );
	}

	m_CommandList->Dispatch( numGroupsX, numGroupsY, numGroupsZ );
}

void CommandList::ClearRenderTargetView(
	D3D12_CPU_DESCRIPTOR_HANDLE rtv, const float clearColor[4])
{
	m_CommandList->ClearRenderTargetView( rtv, clearColor, 0u, nullptr );
}

void CommandList::ClearDepthStencilView(
	D3D12_CPU_DESCRIPTOR_HANDLE dsv, D3D12_CLEAR_FLAGS clearFlags, 
	float depth, u8 stencil)
{
	m_CommandList->ClearDepthStencilView( dsv, clearFlags, depth, stencil, 0, nullptr );
}

void CommandList::UploadBufferData(Buffer& buffer, const void* data, size_t sizeInBytes)
{
	// Transition buffer to copy dest
	TransitionBarrier( buffer.D3D12Resource(), D3D12_RESOURCE_STATE_COPY_DEST );
	FlushResourceBarriers();

	// Create temporary upload buffer
	auto device = App.Renderer().D3D12Device();
	CD3DX12_HEAP_PROPERTIES uploadProps( D3D12_HEAP_TYPE_UPLOAD );
	CD3DX12_RESOURCE_DESC bufferDesc = CD3DX12_RESOURCE_DESC::Buffer( sizeInBytes );

	ComPtr<ID3D12Resource> uploadBuffer;
	log::ThrowIfFailed(
		device->CreateCommittedResource(
			&uploadProps,
			D3D12_HEAP_FLAG_NONE,
			&bufferDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &uploadBuffer )
		)
	);

	// Map and copy
	void* mappedData = nullptr;
	uploadBuffer->Map( 0u, nullptr, &mappedData );
	memcpy( mappedData, data, sizeInBytes );
	uploadBuffer->Unmap( 0u, nullptr );

	// Copy to GPU buffer
	m_CommandList->CopyBufferRegion(
		buffer.D3D12Resource().Get(), 0ull, uploadBuffer.Get(), 0ull, sizeInBytes
	);

	// Track upload buffer so it stays alive until GPU finishes
	TrackObject( uploadBuffer );
	TrackResource( buffer );
}
