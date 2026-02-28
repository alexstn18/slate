#include "pch.hpp"
#include "rendering/upload_buffer.hpp"
#include "util/math.hpp"

using namespace slate;

UploadBuffer::UploadBuffer(size_t pageSize)
	: m_PageSize{ pageSize }
{
}

UploadBuffer::~UploadBuffer()
{
}

// NOTE: allocations for constant buffers must be aligned to 256 bytes
UploadBuffer::Allocation UploadBuffer::Allocate(size_t sizeInBytes, size_t alignment)
{
	if ( sizeInBytes > m_PageSize )
	{
		throw std::bad_alloc();
	}

	if ( !m_CurrentPage || !m_CurrentPage->HasSpace( sizeInBytes, alignment ) )
	{
		m_CurrentPage = RequestPage();
	}

	return m_CurrentPage->Allocate( sizeInBytes, alignment );
}

void UploadBuffer::Reset()
{
	m_CurrentPage = nullptr;
	// reset all available pages
	m_AvailablePages = m_PagePool;

	for ( auto page : m_AvailablePages )
	{
		// reset the page for new allocs
		page->Reset();
	}
}

std::shared_ptr<UploadBuffer::Page> UploadBuffer::RequestPage()
{
	std::shared_ptr<Page> page{};
	
	if ( !m_AvailablePages.empty() )
	{
		page = m_AvailablePages.front();
		m_AvailablePages.pop_front();
	}
	else
	{
		page = std::make_shared<Page>( m_PageSize );
		m_PagePool.push_back( page );
	}

	return page;
}

UploadBuffer::Page::Page(size_t sizeInBytes) : 
	m_PageSize{ sizeInBytes },
	m_Offset{ 0ull },
	m_CPUPtr{ nullptr },
	m_GPUPtr{ D3D12_GPU_VIRTUAL_ADDRESS( 0ull ) }
{
	auto uploadProperty = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_UPLOAD );
	auto resourceDescBuf = CD3DX12_RESOURCE_DESC::Buffer( m_PageSize );

	auto device = App.Renderer().D3D12Device();
	log::ThrowIfFailed(
		device->CreateCommittedResource(
			&uploadProperty,
			D3D12_HEAP_FLAG_NONE,
			&resourceDescBuf,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_PPV_ARGS( &m_D3D12Resource )
		)
	);

	m_GPUPtr = m_D3D12Resource->GetGPUVirtualAddress();
	m_D3D12Resource->Map( 0u, nullptr, &m_CPUPtr );
}

UploadBuffer::Page::~Page()
{
	m_D3D12Resource->Unmap( 0, nullptr );
	m_CPUPtr = nullptr;
	m_GPUPtr = D3D12_GPU_VIRTUAL_ADDRESS( 0ull );
}

bool UploadBuffer::Page::HasSpace(size_t sizeInBytes, size_t alignment) const
{
	size_t alignedSize = math::AlignUp( sizeInBytes, alignment );
	size_t alignedOffset = math::AlignUp( m_Offset, alignment );

	return alignedOffset + alignedSize <= m_PageSize;
}

// This method returns an Allocation structure
// that can be used to directly copy CPU data to GPU and bind
// that GPU address to the pipeline
UploadBuffer::Allocation UploadBuffer::Page::Allocate(size_t sizeInBytes, size_t alignment)
{
	//if (!HasSpace(sizeInBytes, alignment))
	//{
	//	throw std::bad_alloc();
    //}

	size_t alignedSize = math::AlignUp( sizeInBytes, alignment );
	m_Offset = math::AlignUp( m_Offset, alignment );

	Allocation alloc;
	alloc.CPU = static_cast<uint8_t*>( m_CPUPtr ) + m_Offset;
	alloc.GPU = m_GPUPtr + m_Offset;
		
	m_Offset += alignedSize;

	return alloc;
}

void UploadBuffer::Page::Reset()
{
	m_Offset = 0;
}