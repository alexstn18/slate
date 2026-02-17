#include "pch.hpp"
#include "rendering/descriptor_allocator_page.hpp"

using namespace slate;

DescriptorAllocatorPage::DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, 
	u32 numDescriptors)
	: m_HeapType{type}
	, m_NumDescriptorsInHeap{numDescriptors}
{
	auto device = App.Renderer().D3D12Device();

	D3D12_DESCRIPTOR_HEAP_DESC heapDesc = {};
	heapDesc.Type = m_HeapType;
	heapDesc.NumDescriptors = m_NumDescriptorsInHeap;

	log::ThrowIfFailed(device->CreateDescriptorHeap(&heapDesc, IID_PPV_ARGS(&m_D3D12DescriptorHeap)));

	m_BaseDescriptor = m_D3D12DescriptorHeap->GetCPUDescriptorHandleForHeapStart();
	m_DescriptorHandleIncrementSize = device->GetDescriptorHandleIncrementSize(m_HeapType);
	m_NumFreeHandles = m_NumDescriptorsInHeap;

	// Initialize the free lists
	AddNewBlock(0, m_NumFreeHandles);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorAllocatorPage::GetHeapType() const
{
	return m_HeapType;
}

bool DescriptorAllocatorPage::HasSpace(u32 numDescriptors) const
{
	// lower_bound = used to find the first entry in the free list that is not less than
	// the requested number of descriptors
	return m_FreeListBySize.lower_bound(numDescriptors) != m_FreeListBySize.end();
}

u32 DescriptorAllocatorPage::NumFreeHandles() const
{
	return m_NumFreeHandles;
}

DescriptorAllocation DescriptorAllocatorPage::Allocate(u32 numDescriptors)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	// There are less than the requested number of descriptors left in the heap
	// Return a null descriptor and try another heap
	if (numDescriptors > m_NumFreeHandles)
	{
		return DescriptorAllocation();
	}

	// Get the first block that is large enough to satisfy the request
	auto smallestBlockIt = m_FreeListBySize.lower_bound(numDescriptors);
	if (smallestBlockIt == m_FreeListBySize.end())
	{
		// There was no free block that could satisfy the request
		return DescriptorAllocation();
	}

	// size of the smallest block that satisfies the request
	auto blockSize = smallestBlockIt->first;
	// pointer to the same entry in the FreeListByOffset map
	auto offsetIt = smallestBlockIt->second;
	// offset in descriptor heap
	auto offset = offsetIt->first;

	// remove existing free block from free list
	m_FreeListBySize.erase(smallestBlockIt);
	m_FreeListByOffset.erase(offsetIt);

	// compute new free block that results from splitting this block
	auto newOffset = offset + numDescriptors;
	auto newSize = blockSize - numDescriptors;

	if (newSize > 0)
	{
		// if allocation didn't exactly match the requested size,
		// return the left-over to the free list
		AddNewBlock(newOffset, newSize);
	}

	// decrement free handles
	m_NumFreeHandles -= numDescriptors;

	return DescriptorAllocation(
		CD3DX12_CPU_DESCRIPTOR_HANDLE(m_BaseDescriptor, offset, 
		m_DescriptorHandleIncrementSize), numDescriptors, 
		m_DescriptorHandleIncrementSize, shared_from_this());
}

void DescriptorAllocatorPage::Free(DescriptorAllocation&& descriptor, u64 frameNumber)
{
	// compute the offset of the descriptor within the descriptor heap
	auto offset = ComputeOffset(descriptor.GetDescriptorHandle());

	std::lock_guard<std::mutex> lock(m_AllocationMutex);
	
	// don't add the block directly to the free list until the frame has completed
	m_StaleDescriptors.emplace(offset, descriptor.GetNumHandles(), frameNumber);
}

void DescriptorAllocatorPage::ReleaseStaleDescriptors(u64 frameNumber)
{
	std::lock_guard<std::mutex> lock(m_AllocationMutex);

	while (!m_StaleDescriptors.empty() && 
		m_StaleDescriptors.front().FrameNumber <= frameNumber)
	{
		auto& staleDescriptor = m_StaleDescriptors.front();

		// The offset of the descriptor in the heap
		auto offset = staleDescriptor.Offset;
		// The number of descriptors that were allocated
		auto numDescriptors = staleDescriptor.Size;

		FreeBlock(offset, numDescriptors);

		m_StaleDescriptors.pop();
	}
}

u32 DescriptorAllocatorPage::ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle)
{
	return static_cast<u32>(handle.ptr - m_BaseDescriptor.ptr) / m_DescriptorHandleIncrementSize;
}

void DescriptorAllocatorPage::AddNewBlock(u32 offset, u32 numDescriptors)
{
	auto offsetIt = m_FreeListByOffset.emplace(offset, numDescriptors);
	auto sizeIt = m_FreeListBySize.emplace(numDescriptors, offsetIt.first);
	offsetIt.first->second.FreeListBySizeIt = sizeIt;
}

void DescriptorAllocatorPage::FreeBlock(u32 offset, u32 numDescriptors)
{
	// Find the first element whose offset is greater than the specified offset
	// This is the block that should appear after the block that is being freed
	auto nextBlockIt = m_FreeListByOffset.upper_bound(offset);

	// Find the block that appears before the block being freed
	auto prevBlockIt = nextBlockIt;

	// If it's not the first block in the list
	if (prevBlockIt != m_FreeListByOffset.begin())
	{
		// Go to the previous block in the list
		--prevBlockIt;
	}
	else
	{
		// Otherwise, just set it to the end of the list to indicate that no
		// block comes before the one being freed
		prevBlockIt = m_FreeListByOffset.end();
	}

	// Add the number of free handles back to the heap
	// This needs to be done before merging any blocks since merging
	// blocks modifies the numDescriptors variable
	m_NumFreeHandles += numDescriptors;

	if (prevBlockIt != m_FreeListByOffset.end() &&
		offset == prevBlockIt->first + prevBlockIt->second.Size)
	{
		// The previous block is exactly behind the block that is to be freed.
		//
		// PrevBlock.Offset           Offset
		// |                          |
		// |<-----PrevBlock.Size----->|<------Size-------->|
		//

		// Increase the block size by the size of merging with the previous block.
		offset = prevBlockIt->first;
		numDescriptors += prevBlockIt->second.Size;

		// Remove the previous block from the free list
		m_FreeListBySize.erase(prevBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(prevBlockIt);
	}

	if (nextBlockIt != m_FreeListByOffset.end() &&
		offset + numDescriptors == nextBlockIt->first)
	{
		// The next block is exactly in front of the block that is to be freed.
		//
		// Offset               NextBlock.Offset 
		// |                    |
		// |<------Size-------->|<-----NextBlock.Size----->|

		// Increase the block size by the size of merging with the next block
		numDescriptors += nextBlockIt->second.Size;

		// Remove the next block from the free list
		m_FreeListBySize.erase(nextBlockIt->second.FreeListBySizeIt);
		m_FreeListByOffset.erase(nextBlockIt);
	}

	// Add the freed block to the free list
	AddNewBlock(offset, numDescriptors);
}