#include "pch.hpp"
#include "rendering/constant_buffer.hpp"
#include "rendering/descriptor_heap.hpp"

using namespace slate;

std::unique_ptr<ConstantBuffer> ConstantBuffer::Create(size_t sizeInBytes)
{
	auto  device    = App.Renderer().D3D12Device();
	auto& allocator = App.Renderer().D3D12MA_Allocator();

	size_t aligned = ( sizeInBytes + 255 ) & ~255;
	auto   desc = CD3DX12_RESOURCE_DESC::Buffer( aligned );

	D3D12MA::ALLOCATION_DESC allocDesc = {};
	allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

	ComPtr<ID3D12Resource> resource{ nullptr };
	D3D12MA::Allocation* allocation{ nullptr };

	log::ThrowIfFailed(
		allocator.CreateResource(
			&allocDesc,
			&desc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			&allocation,
			IID_PPV_ARGS( &resource )
		)
	);

	auto cb = std::unique_ptr<ConstantBuffer>( new ConstantBuffer( resource ) );
	cb->m_Allocation = allocation;
	cb->m_SizeInBytes = aligned;

	CD3DX12_RANGE readRange( 0ull, 0ull );
	cb->m_D3D12Resource->Map( 0u, &readRange, &cb->m_MappedData );

	auto& srvDescHeap = App.Renderer().GetSRVDescriptorHeap();
	u32 slot = srvDescHeap.GetNextIndex();
	
	auto cpuHandle  = srvDescHeap.GetCPUHandle( slot );
	cb->m_GPUHandle = srvDescHeap.GetGPUHandle( slot );

	D3D12_CONSTANT_BUFFER_VIEW_DESC cbvDesc = {};
	cbvDesc.BufferLocation = resource->GetGPUVirtualAddress();
	cbvDesc.SizeInBytes = UINT( aligned );
	device->CreateConstantBufferView( &cbvDesc, cpuHandle );

	return cb;
}

void ConstantBuffer::SetData(const void* data, size_t sizeInBytes)
{
	assert( m_MappedData && sizeInBytes <= m_SizeInBytes );
	memcpy( m_MappedData, data, sizeInBytes );
}

ConstantBuffer::ConstantBuffer(ComPtr<ID3D12Resource> resource)
	: Buffer{ resource }
{
	m_SizeInBytes = GetResourceDesc().Width;
}

ConstantBuffer::~ConstantBuffer()
{
	if ( m_MappedData ) {
		m_D3D12Resource->Unmap( 0u, nullptr );
		m_MappedData = nullptr;
	}
}
