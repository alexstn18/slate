#include "pch.hpp"
#include "rendering/command_list.hpp"
#include "rendering/resource.hpp"
#include "rendering/upload_buffer.hpp"
#include "rendering/resource_state_tracker.hpp"
#include "rendering/dynamic_descriptor_heap.hpp"

using namespace slate;

void CommandList::Initialize(D3D12_COMMAND_LIST_TYPE type)
{
	auto device = App.Device().GetDevice();

	log::ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&m_CommandAllocator)));
	log::ThrowIfFailed(device->CreateCommandList(0, type, m_CommandAllocator.Get(), nullptr, IID_PPV_ARGS(&m_CommandList)));
}

void CommandList::Reset()
{
	log::ThrowIfFailed(m_CommandAllocator->Reset());
	log::ThrowIfFailed(m_CommandList->Reset(m_CommandAllocator.Get(), nullptr)); // nullptr = Pipeline State Object, optional
}

void CommandList::Close()
{
	log::ThrowIfFailed(m_CommandList->Close());
}

void CommandList::TransitionBarrier(const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT subResource, bool flushBarriers)
{
	auto d3d12Resource = resource.D3D12Resource();
	if (d3d12Resource) {
		// The "before" state is not important
		// It will be resolved the resource state tracker
		auto barrier = CD3DX12_RESOURCE_BARRIER::Transition(d3d12Resource.Get(), D3D12_RESOURCE_STATE_COMMON, stateAfter, subResource);

		m_ResourceStateTracker->ResourceBarrier(barrier);
	}

	if (flushBarriers) {
		FlushResourceBarriers();
	}
}

void CommandList::UAVBarrier(const Resource& resource, bool flushBarriers)
{
	auto d3d12Resource = resource.D3D12Resource();
	if (d3d12Resource) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::UAV(d3d12Resource.Get());

		m_ResourceStateTracker->ResourceBarrier(barrier);

		if (flushBarriers) {
			FlushResourceBarriers();
		}
	}
}

void CommandList::AliasingBarrier(const Resource& beforeRes, const Resource& afterRes, bool flushBarriers)
{
	auto d3d12BeforeRes = beforeRes.D3D12Resource();
	auto d3d12AfterRes = afterRes.D3D12Resource();
	if (d3d12BeforeRes && d3d12AfterRes) {
		auto barrier = CD3DX12_RESOURCE_BARRIER::Aliasing(d3d12BeforeRes.Get(), d3d12AfterRes.Get());

		m_ResourceStateTracker->ResourceBarrier(barrier);

		if (flushBarriers) {
			FlushResourceBarriers();
		}
	}
}

void CommandList::CopyResource(Resource& dstRes, const Resource& srcRes)
{
	TransitionBarrier(dstRes, D3D12_RESOURCE_STATE_COPY_DEST);
	TransitionBarrier(srcRes, D3D12_RESOURCE_STATE_COPY_SOURCE);

	FlushResourceBarriers();

	m_CommandList->CopyResource(dstRes.D3D12Resource().Get(), srcRes.D3D12Resource().Get());

	TrackResource(dstRes);
	TrackResource(srcRes);
}

void CommandList::SetGraphicsDynamicConstantBuffer(u32 rootParameterIndex, size_t sizeInBytes, const void* bufferData)
{
	// Constant buffers must be 256-byte aligned
	auto heapAllocation{ m_UploadBuffer->Allocate(sizeInBytes, D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT) };
	memcpy(heapAllocation.CPU, bufferData, sizeInBytes);

	m_CommandList->SetGraphicsRootConstantBufferView(rootParameterIndex, heapAllocation.GPU);
}

void CommandList::SetShaderResourceView(u32 rootParameterIndex, u32 descriptorOffset, const Resource& resource, D3D12_RESOURCE_STATES stateAfter, UINT firstSubResource, UINT numSubResources, const D3D12_SHADER_RESOURCE_VIEW_DESC* srv)
{
	if (numSubResources < D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES) {
		for (u32 i{ 0u }; i < numSubResources; ++i) {
			TransitionBarrier(resource, stateAfter, firstSubResource + i);
		}
	}
	else {
		TransitionBarrier(resource, stateAfter);
	}

	m_DynamicDescriptorHeap[D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV]->StageDescriptors(rootParameterIndex, descriptorOffset, 1, resource.GetShaderResourceView(srv));

	TrackResource(resource);
}

void CommandList::Draw(u32 vertexCount, u32 instanceCount, u32 startVertex, u32 startInstance)
{
	FlushResourceBarriers();

	for (i32 i{ 0 }; i < D3D12_DESCRIPTOR_HEAP_TYPE_NUM_TYPES; ++i) {
		m_DynamicDescriptorHeap[i]->CommitStagedDescriptorsForDraw(*this);
	}

	m_CommandList->DrawInstanced(vertexCount, instanceCount, startVertex, startInstance);
}
