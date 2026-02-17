#include "pch.hpp"
#include "rendering/mesh.hpp"
#include "rendering/vertex_buffer.hpp"
#include "rendering/index_buffer.hpp"
#include "rendering/command_list.hpp"
#include "rendering/command_queue.hpp"

using namespace slate;

Mesh::Mesh(const std::vector<Vertex>& vertices,
           const std::vector<u32>& indices)
    : m_Vertices(vertices.data(), vertices.data() + vertices.size())
    , m_Indices(indices.data(), indices.data() + indices.size())
{
}

void Mesh::CreateBuffers(CommandList& commandList)
{
    const auto vertexCount = GetVertexCount();
    const auto indexCount = GetIndexCount();

    m_VertexBuffer = std::make_shared<VertexBuffer>(vertexCount, sizeof(Vertex));
    m_IndexBuffer = std::make_shared<IndexBuffer>(indexCount, DXGI_FORMAT_R32_UINT);

    commandList.UploadBufferData(*m_VertexBuffer, m_Vertices.data(), vertexCount);
    commandList.UploadBufferData(*m_IndexBuffer, m_Indices.data(), indexCount);

    m_Vertices.clear();
    m_Vertices.shrink_to_fit();
    m_Indices.clear();
    m_Indices.shrink_to_fit();
}