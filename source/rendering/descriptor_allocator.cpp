#include "pch.hpp"
#include "rendering/descriptor_allocator.hpp"

using namespace slate;

DescriptorAllocator::DescriptorAllocator(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptorsPerHeap) :
	m_Type{type},
	m_NumDescriptorsPerHeap{numDescriptorsPerHeap}
{
}

DescriptorAllocator::~DescriptorAllocator()
{
}

DescriptorAllocation DescriptorAllocator::Allocate(u32 numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);
	DescriptorAllocation descAlloc{};

	for(auto iter = m_AvailableHeaps.begin(); iter != m_AvailableHeaps.end(); ++iter)
	{
		auto allocatorPage = m_HeapPool[*iter];
		descAlloc = allocatorPage->Allocate(numDescriptors);
		if(allocatorPage->NumFreeHandles() == 0)
		{
			iter = m_AvailableHeaps.erase(iter);
		}

		if(!descAlloc.IsNull())
		{
			break;
		}

		if(descAlloc.IsNull())
		{
			m_NumDescriptorsPerHeap = std::max(m_NumDescriptorsPerHeap, numDescriptors);
			auto newPage = CreateAllocatorPage();
			descAlloc = newPage->Allocate(numDescriptors);
		}
	}

	return descAlloc;
}

void DescriptorAllocator::ReleaseStaleDescriptors(u64 frameNumber)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	for(size_t i = 0; i < m_HeapPool.size(); ++i)
	{
		auto page = m_HeapPool[i];
		page->ReleaseStaleDescriptors(frameNumber);
		if(page->NumFreeHandles() > 0)
		{
			m_AvailableHeaps.insert(i);
		}
	}
}

std::shared_ptr<DescriptorAllocatorPage> DescriptorAllocator::CreateAllocatorPage()
{
	auto newPage = std::make_shared<DescriptorAllocatorPage>(m_Type, m_NumDescriptorsPerHeap);
	m_HeapPool.emplace_back(newPage);
	m_AvailableHeaps.insert(m_HeapPool.size() - 1);

	return newPage;
}
