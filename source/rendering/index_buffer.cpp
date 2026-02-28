#include "pch.hpp"
#include "rendering/index_buffer.hpp"

using namespace slate;

IndexBuffer::IndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat)
    : Buffer{ CD3DX12_RESOURCE_DESC::Buffer( 
        numIndices * ( indexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4 ) ) }
    , m_NumIndices { numIndices  }
    , m_IndexFormat{ indexFormat }
    , m_IndexBufferView{}
{
    assert( indexFormat == DXGI_FORMAT_R16_UINT || indexFormat == DXGI_FORMAT_R32_UINT );
    CreateIndexBufferView();
}

IndexBuffer::IndexBuffer(
    ComPtr<ID3D12Resource> resource, size_t numIndices, DXGI_FORMAT indexFormat)
    : Buffer{ resource }
    , m_NumIndices{ numIndices }
    , m_IndexFormat{ indexFormat }
    , m_IndexBufferView{}
{
    assert( indexFormat == DXGI_FORMAT_R16_UINT || indexFormat == DXGI_FORMAT_R32_UINT );
    CreateIndexBufferView();
}

void IndexBuffer::CreateIndexBufferView()
{
    UINT bufferSize = UINT(
        m_NumIndices * ( m_IndexFormat == DXGI_FORMAT_R16_UINT ? 2 : 4 )
    );

    m_IndexBufferView.BufferLocation = m_D3D12Resource->GetGPUVirtualAddress();
    m_IndexBufferView.SizeInBytes = bufferSize;
    m_IndexBufferView.Format = m_IndexFormat;
}