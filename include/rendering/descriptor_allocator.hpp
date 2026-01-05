#pragma once

#include <set>
#include <mutex>

#include "rendering/descriptor_allocation.hpp"

namespace slate
{
	class DescriptorAllocatorPage;

	class DescriptorAllocator
	{
	public:
		DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptorsPerHeap = 256u);

		virtual ~DescriptorAllocator();

		DescriptorAllocation Allocate(u32 numDescriptors = 1u);

		void ReleaseStaleDescriptors(u64 frameNumber);
	private:
		using DescriptorHeapPool = std::vector<std::shared_ptr<DescriptorAllocatorPage>>;

		std::shared_ptr<DescriptorAllocatorPage> CreateAllocatorPage();

		D3D12_DESCRIPTOR_HEAP_TYPE m_Type{};

		u32 m_NumDescriptorsPerHeap{};

		DescriptorHeapPool m_HeapPool{};

		std::set<size_t> m_AvailableHeaps{};

		std::mutex m_AllocationMutex{};
	};
}

