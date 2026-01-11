#pragma once

#include <queue>
#include <functional>

namespace slate
{
	// Purpose: allocating GPU visible descriptors that are used for binding
	// CBV, SRV, UAV and samplers to the GPU pipeline for rendering or
	// compute invocations
	//
	// This is necessary since the descriptors provided by the DescriptorAllocator
	// class shown in the previous section are CPU visible and cannot be used to
	// bind resources to the GPU rendering pipeline
	//
	// The DynamicDescriptorHeap class provides a staging area for GPU visible
	// descriptors that are committed to GPU visible descriptor heaps when a
	// Draw or Dispatch method is invoked on the command list
	//
	// It also ensures that the currently bound descriptor heap has a sufficient number
	// of descriptors to commit all of the staged descriptors before a Draw or Dispatch
	// command is executed. If the currently bound descriptor runs out of descriptors
	// then a new descriptor heap is bound to the command list

	class CommandList;
	class RootSignature;

	class DynamicDescriptorHeap
	{
	public:
		DynamicDescriptorHeap(
			D3D12_DESCRIPTOR_HEAP_TYPE type, 
			u32 numDescriptorsPerHeap = 1024u);
	
		virtual ~DynamicDescriptorHeap();

		void StageDescriptors(u32 rootParameterIndex,
			u32 offset,
			u32 numDescriptors,
			const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor);

		void CommitStagedDescriptors(CommandList& commandList,
			std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc);
		void CommitStagedDescriptorsForDraw(CommandList& commandList);
		void CommitStagedDescriptorsForDispatch(CommandList& commandList);

		// Used to copy a single CPU visible descriptor to a GPU visible descriptor heap
		D3D12_GPU_DESCRIPTOR_HANDLE CopyDescriptor(CommandList& commandList,
			D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor);

		void ParseRootSignature(const RootSignature& rootSignature);

		// Called when the commands that are referencing any descriptor in DynamicDescriptorHeap
		// have finished executing on the GPU
		//
		// When the DDH is reset, all of the descriptor heaps are made available again
		// and the descriptor table cache is reset
		void Reset();
	
	private:
		ComPtr<ID3D12DescriptorHeap> RequestDescriptorHeap();

		ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap();

		u32 ComputeStaleDescriptorCount() const;

		static const u32 MaxDescriptorTables = 32u;

		// Structure that represents a descriptor table entry in the rootsignature
		struct DescriptorTableCache
		{
			DescriptorTableCache()
				: NumDescriptors{0}
				, BaseDescriptor{nullptr}
			{}

			// Reset the table cache
			void Reset()
			{
				NumDescriptors = 0;
				BaseDescriptor = nullptr;
			}

			// The number of descriptors in this descriptor table
			u32 NumDescriptors;
			// The pointer to the descriptor in the descriptor handle cache
			D3D12_CPU_DESCRIPTOR_HANDLE* BaseDescriptor;
		};

		D3D12_DESCRIPTOR_HEAP_TYPE m_DescriptorHeapType{};

		u32 m_NumDescriptorsPerHeap{};
		u32 m_DescriptorHandleIncrementSize{};
		
		std::unique_ptr<D3D12_CPU_DESCRIPTOR_HANDLE[]> m_DescriptorHandleCache{ nullptr };

		DescriptorTableCache m_DescriptorTableCache[MaxDescriptorTables];

		u32 m_DescriptorTableBitMask{};

		u32 m_StaleDescriptorTableBitMask{};

		using DescriptorHeapPool = std::queue<ComPtr<ID3D12DescriptorHeap>>;
		DescriptorHeapPool m_DescriptorHeapPool;
		DescriptorHeapPool m_AvailableDescriptorHeaps;
		
		ComPtr<ID3D12DescriptorHeap> m_CurrentDescriptorHeap{ nullptr };
		CD3DX12_GPU_DESCRIPTOR_HANDLE m_CurrentGPUDescriptorHandle;
		CD3DX12_CPU_DESCRIPTOR_HANDLE m_CurrentCPUDescriptorHandle;

		u32 m_NumFreeHandles{};
	};
}

