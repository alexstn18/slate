#pragma once

#include "rendering/buffer.hpp"

namespace slate
{
	class IndexBuffer : public Buffer
	{
	public:
		IndexBuffer(size_t numIndices, DXGI_FORMAT indexFormat);
		IndexBuffer(ComPtr<ID3D12Resource> resource, size_t numIndices, DXGI_FORMAT indexFormat);
		virtual ~IndexBuffer() = default;
		D3D12_INDEX_BUFFER_VIEW GetIndexBufferView() const { return m_IndexBufferView; }
		size_t GetNumIndices() const { return m_NumIndices; }
		DXGI_FORMAT GetIndexFormat() const { return m_IndexFormat; }

	protected:

		void CreateIndexBufferView();
	private:
		size_t m_NumIndices{};
		DXGI_FORMAT m_IndexFormat{};
		D3D12_INDEX_BUFFER_VIEW m_IndexBufferView{};
	};
}