#include "pch.hpp"
#include "rendering/mesh.hpp"
#include "rendering/vertex_buffer.hpp"
#include "rendering/index_buffer.hpp"
#include "rendering/command_list.hpp"
#include "rendering/command_queue.hpp"

using namespace slate;

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<u32>& indices,
           const Material& material)
    : m_Vertices( vertices.data(), vertices.data() + vertices.size() )
    , m_Indices( indices.data(), indices.data() + indices.size() )
    , m_Material( std::make_shared<Material>( material ) )
{
}

void Mesh::CreateBuffers(CommandList& commandList)
{
    const auto vertexCount = GetVertexCount();
    const auto indexCount = GetIndexCount();

    m_VertexBuffer = std::make_shared<VertexBuffer>( vertexCount, sizeof( Vertex ) );
    m_IndexBuffer = std::make_shared<IndexBuffer>( indexCount, DXGI_FORMAT_R32_UINT );

    commandList.UploadBufferData(
        *m_VertexBuffer, m_Vertices.data(), vertexCount * sizeof( Vertex )
    );
    commandList.UploadBufferData(
        *m_IndexBuffer, m_Indices.data(), indexCount * sizeof( u32 )
    );

    log::Info( "Uploading {} vertices ({} bytes), {} indices ({} bytes)",
        vertexCount, vertexCount * sizeof( Vertex ),
        indexCount, indexCount * sizeof( uint32_t )
    );
}