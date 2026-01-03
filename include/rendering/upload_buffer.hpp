#pragma once
namespace slate
{
	class UploadBuffer
	{
	public:
		// Used to upload data to GPU
		struct Allocation
		{
			void* CPU{};
			D3D12_GPU_VIRTUAL_ADDRESS GPU{};
		};

		// Size to allocate new pages in GPU memory
		// 2MB is the default size of a page of memory
		// Memory page size should be approx. large enough
		// to contain all of the allocations for a single CommandList
		explicit UploadBuffer(size_t pageSize = _2MB);
		// If a lot of dynamic memory alloc is happening inside a CL
		// consider increasing the pagesize

		virtual ~UploadBuffer();

		// Maximum size of an allocation is the pagesize
		size_t GetPageSize() const { return m_PageSize; }

		// Allocate memory in an upload heap
		// An allocation must not exceed the pagesize
		Allocation Allocate(size_t sizeInBytes, size_t alignment);

		// Release all allocated pages
		// NOTICE: this function should only be called when
		// the CL is finished executing on the CommandQueue
		void Reset();
	private:
		// Single page for allocator
		struct Page
		{
			Page(size_t sizeInBytes);
			~Page();

			bool HasSpace(size_t sizeInBytes, size_t alignment) const;

			// Allocate memory from the page
			// throws std::bad_alloc if allocation size is larger than the page size
			// or the size of the alloc exceeds the remaining space in the page
			Allocation Allocate(size_t sizeInBytes, size_t alignment);

			void Reset();
		private:
			ComPtr<ID3D12Resource> m_D3D12Resource{ nullptr };

			void* m_CPUPtr{ nullptr };
			D3D12_GPU_VIRTUAL_ADDRESS m_GPUPtr{};

			size_t m_PageSize{};
			size_t m_Offset{};
		};

		using PagePool = std::deque<std::shared_ptr<Page>>;

		std::shared_ptr<Page> RequestPage();
		
		PagePool m_PagePool{};
		PagePool m_AvailablePages{};

		std::shared_ptr<Page> m_CurrentPage{};

		size_t m_PageSize{};
	};
}

