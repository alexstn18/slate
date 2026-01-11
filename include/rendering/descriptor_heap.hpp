#pragma once

namespace slate
{
	enum class HeapType : u32
	{
		RTV,
		DSV,
		SRV,
		SMP,
	};

	class DescriptorHeap
	{
	public:
		void Initialize(HeapType type, u32 maxDescriptors);

		uint32_t GetNextIndex();
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(u32 index);
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(u32 index);

		inline void Reset() { m_CurrentIndex = 0; }

		[[nodiscard]] ComPtr<ID3D12DescriptorHeap> Get() const noexcept { return m_Heap; }
		bool HasSpace() const noexcept { return m_CurrentIndex < m_MaxIndex; }
		uint32_t GetAvailableCount() const noexcept { return m_MaxIndex - m_CurrentIndex; }
	protected:
		friend class Device;
	private:
		D3D12_DESCRIPTOR_HEAP_TYPE HeapTypeToD3D12HeapType(HeapType type);
		const char* HeapTypeAsString(HeapType type);

		ComPtr<ID3D12DescriptorHeap> m_Heap{ nullptr };

		u32 m_DescriptorSize{ 0 };
		u32 m_CurrentIndex{ 0 };
		u32 m_MaxIndex{ 0 };
	};
}
