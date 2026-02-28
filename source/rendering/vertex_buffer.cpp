#include "pch.hpp"
#include "rendering/vertex_buffer.hpp"

using namespace slate;

VertexBuffer::VertexBuffer(size_t numVertices, size_t vertexStride)
	: Buffer(CD3DX12_RESOURCE_DESC::Buffer(numVertices* vertexStride))
	, m_NumVertices{ numVertices }
	, m_VertexStride{ vertexStride }
	, m_VertexBufferView{}
{
	CreateVertexBufferView();
}

VertexBuffer::VertexBuffer(ComPtr<ID3D12Resource> resource, size_t numVertices, size_t vertexStride)
	: Buffer{ resource }
	, m_NumVertices{ numVertices }
	, m_VertexStride{ vertexStride }
	, m_VertexBufferView{}
{
	CreateVertexBufferView();
}

VertexBuffer::~VertexBuffer()
{
}

void VertexBuffer::CreateVertexBufferView()
{
	m_VertexBufferView.BufferLocation = m_D3D12Resource->GetGPUVirtualAddress();
	m_VertexBufferView.SizeInBytes = static_cast<UINT>( m_NumVertices * m_VertexStride );
	m_VertexBufferView.StrideInBytes = static_cast<UINT>( m_VertexStride );
}
