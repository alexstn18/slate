#pragma once

namespace slate
{
	enum class HeapType : uint32_t
	{
		RTV,
		DSV,
		SRV,
		SMP,
	};

	class DescriptorHeap
	{
	public:
		void Initialize(HeapType type, uint32_t maxDescriptors);

		uint32_t GetNextIndex();
		CD3DX12_CPU_DESCRIPTOR_HANDLE GetCPUHandle(uint32_t index);
		CD3DX12_GPU_DESCRIPTOR_HANDLE GetGPUHandle(uint32_t index);

		inline void Reset() { m_CurrentIndex = 0; }

		[[nodiscard]] ComPtr<ID3D12DescriptorHeap> Get() const { return m_Heap; }
		bool HasSpace() const { return m_CurrentIndex < m_MaxIndex; }
		uint32_t GetAvailableCount() const { return m_MaxIndex - m_CurrentIndex; }
	protected:
		friend class Device;
	private:
		D3D12_DESCRIPTOR_HEAP_TYPE HeapTypeToD3D12HeapType(HeapType type);
		const char* HeapTypeAsString(HeapType type);

		ComPtr<ID3D12DescriptorHeap> m_Heap{ nullptr };

		uint32_t m_DescriptorSize{ 0 };
		uint32_t m_CurrentIndex{ 0 };
		uint32_t m_MaxIndex{ 0 };
	};
}
