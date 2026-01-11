#pragma once

#include "rendering/buffer.hpp"

namespace slate {
	class VertexBuffer : public Buffer {
	public:
		VertexBuffer(size_t numVertices, size_t vertexStride);
		VertexBuffer(ComPtr<ID3D12Resource> resource, size_t numVertices, size_t vertexStride);
		virtual ~VertexBuffer();
		D3D12_VERTEX_BUFFER_VIEW GetVertexBufferView() const
		{
			return m_VertexBufferView;
		}

		size_t GetNumVertices() const
		{
			return m_NumVertices;
		}

		size_t GetVertexStride() const
		{
			return m_VertexStride;
		}
	protected:
		void CreateVertexBufferView();
	private:
		size_t                   m_NumVertices;
		size_t                   m_VertexStride;
		D3D12_VERTEX_BUFFER_VIEW m_VertexBufferView;
	};
}

