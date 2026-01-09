#pragma once

#include <map>
#include <mutex>
#include <queue>

#include "descriptor_allocation.hpp"

namespace slate
{
	// Class to provide free list allocator strategy for ID3D12DescriptorHeap
	// Not intended to be used outside of DescriptorAllocator class
	class DescriptorAllocatorPage : public std::enable_shared_from_this<DescriptorAllocatorPage>
	{
	public:
		DescriptorAllocatorPage(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptors);

		D3D12_DESCRIPTOR_HEAP_TYPE GetHeapType() const;

		// Check to see if DescriptorAllocatorPage has a contiguous block of descriptors
		// that is large enough to satisfy a request
		bool HasSpace(u32 numDescriptors) const;

		// Get the number of available handles in the heap
		u32 NumFreeHandles() const;

		// Allocate a number of descriptors from this descriptor heap
		// If the allocation can't be satisfied, return a null descriptor
		DescriptorAllocation Allocate(u32 numDescriptors);

		// Return descriptor back to the heap
		// Stale descriptors are not freed directly, but put on a stale allocations queue
		// Stale allocations are returned to the heap using
		// DescriptorAllocatorPage::ReleaseStaleAllocations function
		// USED TOGETHER WITH DescriptorAllocatorPage::Allocate
		void Free(DescriptorAllocation&& descriptor, u64 frameNumber);

		// Returned the stale descriptors back to the descriptor heap
		// NOTICE: stale descriptors are returned to the free list
		// using the ReleaseStaleDescriptors method when the frame that they were
		// freed in is finished executing on the GPU
		void ReleaseStaleDescriptors(u64 frameNumber);
	protected:
		// Compute offset of descriptor handle from the start of the heap 
		// This function is used to determine where a descriptor needs to be placed
		// back in heap when the descriptor is freed
		//
		// NOTICE: used by the Free method in order to compute the offset
		// of a descriptor in the descriptor heap
		u32 ComputeOffset(D3D12_CPU_DESCRIPTOR_HANDLE handle);
		
		// Adds a new block to the free list
		// This function is used to initialize the free list when splitting
		// a block of descriptors during allocation (block that has all descriptors)
		// and for merging neighboring blocks when descriptors are freed
		void AddNewBlock(u32 offset, u32 numDescriptors);

		// Free a block of descriptors
		// This will also merge free blocks in the free list to form larger blocks
		// that can be reused
		// This function is used by the ReleaseStaleDescriptors method to commit
		// the stale descriptors back to the descriptor heap
		// This method also checks if neighboring blocks in the free list can be merged
		//
		// NOTICE: Merging free blocks in the free list reduces the fragmentation
		// in the free list
		void FreeBlock(u32 offset, u32 numDescriptors);
	private:
		// The offset (in descriptors) within the descriptor heap
		using OffsetType = u32;
		// The number of descriptors that are available
		using SizeType = u32;

		struct FreeBlockInfo;

		// Map that lists the free blocks by the offset within the descriptor heap
		using FreeListByOffset = std::map<OffsetType, FreeBlockInfo>;
		// Map that lists the free blocks by size
		// Needs to be a multimap since multiple blocks can have the same size
		using FreeListBySize = std::multimap<SizeType, FreeListByOffset::iterator>;

		// Stores the size of the block in the free list and a ref (iterator)
		// to its entry in the FreeListBySize map (so that the entry can be quickly
		// removed without searching when merging neighboring blocks in the free list)
		struct FreeBlockInfo
		{
			FreeBlockInfo(SizeType size) 
				: Size{size}
			{}

			SizeType Size;
			FreeListBySize::iterator FreeListBySizeIt;
		};

		// Used to keep track of descriptors in the descriptor heap that have been
		// freed but can't be reused until the frame in which they were freed
		// is finished executing on the GPU
		//
		// NOTICE: this also tracks the offset of the first descriptor and the num
		// of descriptors in the descriptor range. FrameNumber param stores
		// the frame that the descriptors were freed
		struct StaleDescriptorInfo
		{
			StaleDescriptorInfo(OffsetType offset, SizeType size, u64 frameNumber) 
				: Offset{offset}
				, Size{size}
				, FrameNumber{frameNumber}
			{}

			// The offset within the descriptor heap
			OffsetType Offset;

			// The number of descriptors
			SizeType Size;

			// The frame number that the descriptor was freed
			u64 FrameNumber;
		};

		using StaleDescriptorQueue = std::queue<StaleDescriptorInfo>;
		
		FreeListByOffset m_FreeListByOffset{};
		FreeListBySize m_FreeListBySize{};
		StaleDescriptorQueue m_StaleDescriptors{};

		ComPtr<ID3D12DescriptorHeap> m_D3D12DescriptorHeap{ nullptr };
		D3D12_DESCRIPTOR_HEAP_TYPE m_HeapType{};
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_BaseDescriptor{};
		u32 m_DescriptorHandleIncrementSize{};
		u32 m_NumDescriptorsInHeap{};
		u32 m_NumFreeHandles{};

		std::mutex m_AllocationMutex{};
	};
}

