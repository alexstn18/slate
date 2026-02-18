#include "pch.hpp"
#include "rendering/descriptor_heap.hpp"

// Inspired by 
// https://github.com/PappaNiels/IntroDXR/blob/main/code/DXRCore/Renderer/Attributes/DescriptorHeap.cpp

using namespace slate;

void DescriptorHeap::Initialize(HeapType type, u32 maxDescriptors)
{
	auto device = App.Renderer().D3D12Device();

	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	D3D12_DESCRIPTOR_HEAP_TYPE heapType = HeapTypeToD3D12HeapType(type);
	D3D12_DESCRIPTOR_HEAP_FLAGS heapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;

	if (heapType == D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV ||
		heapType == D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER)
	{
		heapFlags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	}

	desc.Type = heapType;
	desc.Flags = heapFlags;
	desc.NodeMask = 0;
	desc.NumDescriptors = maxDescriptors;

	log::ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&m_Heap)));

	m_DescriptorSize = device->GetDescriptorHandleIncrementSize(heapType);
	m_MaxIndex = maxDescriptors;
	m_CurrentIndex = 1;
	log::Info("Created descriptor heap: type={}, descriptors={}, size={}", HeapTypeAsString(type), maxDescriptors, m_DescriptorSize);
}

uint32_t DescriptorHeap::GetNextIndex()
{
	log::Assert(m_CurrentIndex < m_MaxIndex, "Descriptor heap is full ({}/{})",
		m_CurrentIndex, m_MaxIndex);

	return m_CurrentIndex++;
}

CD3DX12_CPU_DESCRIPTOR_HANDLE DescriptorHeap::GetCPUHandle(uint32_t index)
{
	log::Assert(index < m_MaxIndex, "Descriptor index {} out of bounds (max: {})", index, m_MaxIndex);
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_Heap->GetCPUDescriptorHandleForHeapStart(), i32(index), m_DescriptorSize);
}

CD3DX12_GPU_DESCRIPTOR_HANDLE DescriptorHeap::GetGPUHandle(uint32_t index)
{
	log::Assert(m_Heap->GetDesc().Flags & D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE,
		"Heap must be shader visible to get GPU handle");

	return CD3DX12_GPU_DESCRIPTOR_HANDLE(m_Heap->GetGPUDescriptorHandleForHeapStart(), i32(index), m_DescriptorSize);
}

D3D12_DESCRIPTOR_HEAP_TYPE DescriptorHeap::HeapTypeToD3D12HeapType(HeapType type)
{
	switch (type)
	{
	case HeapType::RTV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	case HeapType::SRV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	case HeapType::DSV:
		return D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	case HeapType::SMP:
		return D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	default:
	{
		log::Critical("Invalid heap type");
		return D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	}
	}
}

const char* DescriptorHeap::HeapTypeAsString(HeapType type)
{
	switch (type)
	{
	case HeapType::RTV:
		return "RTV";
	case HeapType::SRV:
		return "SRV/CBV/UAV";
	case HeapType::DSV:
		return "DSV";
	case HeapType::SMP:
		return "SMP";
	default:
	{
		log::Critical("Invalid heap type");
		return "";
	}
	}
}
