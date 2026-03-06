#pragma once

#include "rendering/buffer.hpp"

namespace slate {
	class StructuredBuffer : public Buffer {
	public:
		static std::unique_ptr<StructuredBuffer> Create(
			u32 elementCount, u32 stride, void* data = nullptr);

		D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(
			const D3D12_SHADER_RESOURCE_VIEW_DESC* = nullptr) const override {
			return m_SRVHandle;
		}
		D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* = nullptr) const override {
			return m_UAVHandle;
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUSRVHandle() const noexcept { return m_GPUSRVHandle; }
		u32 GetElementCount() const noexcept { return m_ElementCount; }
		u32 GetStride()       const noexcept { return m_Stride; }

		void SetData(const void* data, size_t sizeInBytes);

		~StructuredBuffer() = default;
	protected:
		StructuredBuffer(ComPtr<ID3D12Resource> resource,
			u32 elementCount,
			u32 stride);
	private:
		u32 m_ElementCount{ 0u };
		u32 m_Stride{ 0u };
		D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle{};
		D3D12_CPU_DESCRIPTOR_HANDLE m_UAVHandle{};
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUSRVHandle{};

		void* m_MappedData{ nullptr };
	};
}

