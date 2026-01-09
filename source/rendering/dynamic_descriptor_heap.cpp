#include "pch.hpp"
#include "rendering/dynamic_descriptor_heap.hpp"
#include "rendering/command_list.hpp"
#include "rendering/root_signature.hpp"


using namespace slate;

DynamicDescriptorHeap::DynamicDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE type, u32 numDescriptorsPerHeap)
	: m_DescriptorHeapType{ type }
	, m_NumDescriptorsPerHeap{ numDescriptorsPerHeap }
	, m_DescriptorTableBitMask{ 0u }
	, m_StaleDescriptorTableBitMask{ 0u }
	, m_CurrentCPUDescriptorHandle{ D3D12_DEFAULT }
	, m_CurrentGPUDescriptorHandle{ D3D12_DEFAULT }
	, m_NumFreeHandles{ 0u }
{
	m_DescriptorHandleIncrementSize = App.Device().GetDescriptorHandleIncrementSize(type);

	m_DescriptorHandleCache = std::make_unique<D3D12_CPU_DESCRIPTOR_HANDLE[]>(m_NumDescriptorsPerHeap);
}

DynamicDescriptorHeap::~DynamicDescriptorHeap()
{
}

void DynamicDescriptorHeap::StageDescriptors(u32 rootParameterIndex,
	u32 offset,
	u32 numDescriptors,
	const D3D12_CPU_DESCRIPTOR_HANDLE srcDescriptor)
{
	// Cannot stage more than the maximum number of descriptors per heap
	// Cannot stage more than MaxDescriptorTables root parameters
	if (numDescriptors > m_NumDescriptorsPerHeap || rootParameterIndex >= MaxDescriptorTables)
	{
		throw std::bad_alloc();
	}

	DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootParameterIndex];

	// Check that the number of descriptors to copy does not exceed the number
	// of descriptors expected in the descriptor table
	if ((offset + numDescriptors) > descriptorTableCache.NumDescriptors)
	{
		throw std::length_error("Number of descriptors exceeds the number of descriptors in the descriptor table");
	}

	D3D12_CPU_DESCRIPTOR_HANDLE* dstDescriptor = (descriptorTableCache.BaseDescriptor + offset);
	for (u32 i = 0; i < numDescriptors; ++i)
	{
		dstDescriptor[i] = CD3DX12_CPU_DESCRIPTOR_HANDLE(srcDescriptor, i, m_DescriptorHandleIncrementSize);
	}

	// Set the root parameter index bit to make sure the descriptor table
	// at that index is bound to the command list
	m_StaleDescriptorTableBitMask |= (1 << rootParameterIndex);
}

void DynamicDescriptorHeap::CommitStagedDescriptors(CommandList& commandList,
	std::function<void(ID3D12GraphicsCommandList*, UINT, D3D12_GPU_DESCRIPTOR_HANDLE)> setFunc)
{
	// Compute the number of descriptors that need to be copied
	u32 numDescriptorsToCommit{ ComputeStaleDescriptorCount() };

	if (numDescriptorsToCommit > 0)
	{
		auto device = App.Device().GetDevice();
		auto d3d12GraphicsCommandList = commandList.Get();
		assert(d3d12GraphicsCommandList != nullptr);

		if (!m_CurrentDescriptorHeap || m_NumFreeHandles < numDescriptorsToCommit)
		{
			m_CurrentDescriptorHeap = RequestDescriptorHeap();
			m_CurrentCPUDescriptorHandle = m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart();
			m_CurrentGPUDescriptorHandle = m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart();
			m_NumFreeHandles = m_NumDescriptorsPerHeap;

			commandList.SetDescriptorHeap(m_DescriptorHeapType, m_CurrentDescriptorHeap.Get());

			// When updating the descriptor heap on the command list,
			// all descriptor tables must be (re)copied to the new
			// descriptor heap (not just the stale descriptor tables)
			m_StaleDescriptorTableBitMask = m_DescriptorTableBitMask;
		}

		DWORD rootIndex{};
		// Scan from LSB to MSB for a bit set in staleDescriptorsBitMask
		while (_BitScanForward(&rootIndex, m_StaleDescriptorTableBitMask))
		{
			UINT numSrcDescriptors{ m_DescriptorTableCache[rootIndex].NumDescriptors };
			D3D12_CPU_DESCRIPTOR_HANDLE* pSrcDescriptorHandles = m_DescriptorTableCache[rootIndex].BaseDescriptor;

			D3D12_CPU_DESCRIPTOR_HANDLE  pDestDescriptorRangeStarts[] =
			{
				m_CurrentCPUDescriptorHandle
			};

			UINT pDestDescriptorRangeSizes[] =
			{
				numSrcDescriptors
			};

			// Copy the staged CPU visible descriptors to the GPU visible descriptor heap
			device->CopyDescriptors(1,
				pDestDescriptorRangeStarts,
				pDestDescriptorRangeSizes,
				numSrcDescriptors,
				pSrcDescriptorHandles,
				nullptr,
				m_DescriptorHeapType);

			// Set the descriptors on the command list used in the passed-in setter 
			setFunc(d3d12GraphicsCommandList.Get(), static_cast<UINT>(rootIndex), m_CurrentGPUDescriptorHandle);

			// Offset current CPU and GPU descriptor handles
			m_CurrentCPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
			m_CurrentGPUDescriptorHandle.Offset(numSrcDescriptors, m_DescriptorHandleIncrementSize);
			m_NumFreeHandles -= numSrcDescriptors;

			// Flip the stale bit so the descriptor table is not recopied
			m_StaleDescriptorTableBitMask ^= (1 << rootIndex);
		}
	}
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDraw(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetGraphicsRootDescriptorTable);
}

void DynamicDescriptorHeap::CommitStagedDescriptorsForDispatch(CommandList& commandList)
{
	CommitStagedDescriptors(commandList, &ID3D12GraphicsCommandList::SetComputeRootDescriptorTable);
}

D3D12_GPU_DESCRIPTOR_HANDLE DynamicDescriptorHeap::CopyDescriptor(CommandList& commandList,
	D3D12_CPU_DESCRIPTOR_HANDLE cpuDescriptor)
{
	if (!m_CurrentDescriptorHeap || m_NumFreeHandles < 1)
	{
		m_CurrentDescriptorHeap = RequestDescriptorHeap();
		
		m_CurrentCPUDescriptorHandle =
		{
			m_CurrentDescriptorHeap->GetCPUDescriptorHandleForHeapStart()
		};
		m_CurrentGPUDescriptorHandle = 
		{
			m_CurrentDescriptorHeap->GetGPUDescriptorHandleForHeapStart()
		}; 

		m_NumFreeHandles = m_NumDescriptorsPerHeap;

		commandList.SetDescriptorHeap(m_DescriptorHeapType, m_CurrentDescriptorHeap.Get());

		// When updating the descriptor heap on the command list,
		// all descriptor tables must be (re)copied to the new
		// descriptor heap (not just the stale descriptor tables)
		m_StaleDescriptorTableBitMask = m_DescriptorTableBitMask;
	}

	auto device = App.Device().GetDevice();
	
	D3D12_GPU_DESCRIPTOR_HANDLE hGPU{ m_CurrentGPUDescriptorHandle };
	device->CopyDescriptorsSimple(1, m_CurrentCPUDescriptorHandle, cpuDescriptor, m_DescriptorHeapType);

	m_CurrentCPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_CurrentGPUDescriptorHandle.Offset(1, m_DescriptorHandleIncrementSize);
	m_NumFreeHandles -= 1u;

	return hGPU;
}

void DynamicDescriptorHeap::ParseRootSignature(const RootSignature& rootSignature)
{
	m_StaleDescriptorTableBitMask = 0u;

	const auto& rootSignatureDesc = rootSignature.GetRootSignatureDesc();

	m_DescriptorTableBitMask = rootSignature.GetDescriptorTableBitMask(m_DescriptorHeapType);
	u32 descriptorTableBitMask = m_DescriptorTableBitMask;
	u32 currentOffset = 0u;
	DWORD rootIndex{};

	while (_BitScanForward(&rootIndex, descriptorTableBitMask) &&
		rootIndex < rootSignatureDesc.NumParameters)
	{
		u32 numDescriptors = rootSignature.GetNumDescriptors(rootIndex);

		DescriptorTableCache& descriptorTableCache = m_DescriptorTableCache[rootIndex];
		descriptorTableCache.NumDescriptors = numDescriptors;
		descriptorTableCache.BaseDescriptor = m_DescriptorHandleCache.get() + currentOffset;

		currentOffset += numDescriptors;

		descriptorTableBitMask ^= (1 << rootIndex);
	}

	// Make sure the maximum number of descriptors per descriptor heap has not been exceeded.
	assert(currentOffset <= m_NumDescriptorsPerHeap &&
		"The root signature requires more than the maximum number of descriptors per descriptor heap. Consider increasing the maximum number of descriptors per descriptor heap.");
}

void DynamicDescriptorHeap::Reset()
{
	m_AvailableDescriptorHeaps = m_DescriptorHeapPool;
	m_CurrentDescriptorHeap.Reset();
	m_CurrentCPUDescriptorHandle = CD3DX12_CPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_CurrentGPUDescriptorHandle = CD3DX12_GPU_DESCRIPTOR_HANDLE(D3D12_DEFAULT);
	m_NumFreeHandles = 0u;
	m_DescriptorTableBitMask = 0u;
	m_StaleDescriptorTableBitMask = 0u;

	// Reset the table cache
	for (i32 i = 0; i < MaxDescriptorTables; ++i)
	{
		m_DescriptorTableCache[i].Reset();
	}
}

ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::RequestDescriptorHeap()
{
	ComPtr<ID3D12DescriptorHeap> descriptorHeap{ nullptr };

	if (!m_AvailableDescriptorHeaps.empty())
	{
		descriptorHeap = m_AvailableDescriptorHeaps.front();
		m_AvailableDescriptorHeaps.pop();
	}
	else
	{
		descriptorHeap = CreateDescriptorHeap();
		m_DescriptorHeapPool.push(descriptorHeap);
	}

	return descriptorHeap;
}

ComPtr<ID3D12DescriptorHeap> DynamicDescriptorHeap::CreateDescriptorHeap()
{
	auto device = App.Device().GetDevice();

	D3D12_DESCRIPTOR_HEAP_DESC descriptorHeapDesc = {};
	descriptorHeapDesc.Type = m_DescriptorHeapType;
	descriptorHeapDesc.NumDescriptors = m_NumDescriptorsPerHeap;
	descriptorHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;

	ComPtr<ID3D12DescriptorHeap> descriptorHeap{ nullptr };
	log::ThrowIfFailed(device->CreateDescriptorHeap(&descriptorHeapDesc, IID_PPV_ARGS(&descriptorHeap)));

	return descriptorHeap;
}

u32 DynamicDescriptorHeap::ComputeStaleDescriptorCount() const
{
	u32 numStaleDescriptors{ 0 };
	DWORD i{};
	DWORD staleDescriptorsBitMask{ m_StaleDescriptorTableBitMask };

	while (_BitScanForward(&i, staleDescriptorsBitMask))
	{
		numStaleDescriptors += m_DescriptorTableCache[i].NumDescriptors;
		staleDescriptorsBitMask ^= (1 << i);
	}

	return numStaleDescriptors;
}
