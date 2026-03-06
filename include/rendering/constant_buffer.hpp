#pragma once
#include "rendering/buffer.hpp"

namespace slate {
	class ConstantBuffer : public Buffer {
	public:
		static std::unique_ptr<ConstantBuffer> Create(size_t sizeInBytes);

		void SetData(const void* data, size_t sizeInBytes);

		D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(
			const D3D12_SHADER_RESOURCE_VIEW_DESC* = nullptr) const override {
			return {};
		}
		D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(
			const D3D12_UNORDERED_ACCESS_VIEW_DESC* = nullptr) const override {
			return {};
		}

		D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept {
			return m_GPUHandle;
		}

		D3D12_GPU_VIRTUAL_ADDRESS GetGPUAddress() const noexcept {
			return m_D3D12Resource->GetGPUVirtualAddress();
		}

		size_t GetSizeInBytes() const { return m_SizeInBytes; }
		virtual ~ConstantBuffer();
	protected:
		ConstantBuffer(ComPtr<ID3D12Resource> resource);
	private:
		void* m_MappedData{ nullptr };
		size_t m_SizeInBytes{ 0ull };
		D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle{};
	};
}

