#include "pch.hpp"
#include "rendering/structured_buffer.hpp"
#include "rendering/resource_state_tracker.hpp"
#include "rendering/descriptor_heap.hpp"
#include "rendering/command_list.hpp"

using namespace slate;

std::unique_ptr<StructuredBuffer> StructuredBuffer::Create(
	u32 elementCount, u32 stride, void* data)
{
    auto& allocator = App.Renderer().D3D12MA_Allocator();
    auto device = App.Renderer().D3D12Device();

    u64 sizeInBytes{ u64( elementCount ) * stride };
    auto desc = CD3DX12_RESOURCE_DESC::Buffer(
        sizeInBytes,
        D3D12_RESOURCE_FLAG_NONE,
        D3D12_DEFAULT_RESOURCE_PLACEMENT_ALIGNMENT
    );

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;

    ComPtr<ID3D12Resource> resource{ nullptr };
    D3D12MA::Allocation* allocation{ nullptr };

    log::ThrowIfFailed(
        allocator.CreateResource(
            &allocDesc,
            &desc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &allocation,
            IID_PPV_ARGS( &resource )
        )
    );

    ResourceStateTracker::AddGlobalResourceState(
        resource.Get(), D3D12_RESOURCE_STATE_GENERIC_READ
    );

    auto sb = std::unique_ptr<StructuredBuffer>(
        new StructuredBuffer( resource, elementCount, stride )
    );

    sb->m_Allocation = allocation;

    CD3DX12_RANGE readRange(0, 0); // no read from CPU
    sb->m_D3D12Resource->Map(0, &readRange, &sb->m_MappedData);

    // SRV
    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Format = DXGI_FORMAT_UNKNOWN;
    srvDesc.Shader4ComponentMapping =
        D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_BUFFER;
    srvDesc.Buffer.FirstElement = 0ull;
    srvDesc.Buffer.NumElements = elementCount;
    srvDesc.Buffer.StructureByteStride = stride;

    auto& srvDescHeap = App.Renderer().GetSRVDescriptorHeap();
    u32 srvSlot = srvDescHeap.GetNextIndex();

    auto cpuHandle = srvDescHeap.GetCPUHandle( srvSlot );
    auto gpuHandle = srvDescHeap.GetGPUHandle( srvSlot );

    device->CreateShaderResourceView(
        resource.Get(),
        &srvDesc,
        cpuHandle
    );

    sb->m_SRVHandle = cpuHandle;
    sb->m_GPUSRVHandle = gpuHandle;

    // UAV
    //D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    //uavDesc.Format = DXGI_FORMAT_UNKNOWN;
    //uavDesc.ViewDimension = D3D12_UAV_DIMENSION_BUFFER;
    //uavDesc.Buffer.FirstElement = 0ull;
    //uavDesc.Buffer.NumElements = elementCount;
    //uavDesc.Buffer.StructureByteStride = stride;

    //u32 uavSlot = srvDescHeap.GetNextIndex();
    //auto uavCPUHandle = srvDescHeap.GetCPUHandle( uavSlot );
    //device->CreateUnorderedAccessView(
    //    resource.Get(),
    //    nullptr,
    //    &uavDesc,
    //    uavCPUHandle
    //);

    //sb->m_UAVHandle = uavCPUHandle;

    if ( data ) {
        sb->SetData(data, sizeInBytes);
        //App.Renderer().GetCommandList()->UploadBufferData(
        //    *sb,
        //    data,
        //    sizeInBytes
        //);
    }

	return sb;
}

void StructuredBuffer::SetData(const void* data, size_t sizeInBytes)
{
    assert(m_MappedData && sizeInBytes <= u64(m_ElementCount) * m_Stride);
    memcpy(m_MappedData, data, sizeInBytes);
}

StructuredBuffer::StructuredBuffer(
    ComPtr<ID3D12Resource> resource, u32 elementCount, u32 stride)
    : Buffer{ resource }
    , m_ElementCount{ elementCount }
    , m_Stride{ stride }
{
}