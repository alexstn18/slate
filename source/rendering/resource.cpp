#include "pch.hpp"
#include "rendering/resource.hpp"
#include "rendering/resource_state_tracker.hpp"

using namespace slate;

Resource::Resource(const std::wstring& name)
    : m_ResourceName(name)
{
}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue, const std::wstring& name)
{
    auto device = App.Renderer().D3D12Device();

    if ( clearValue ) {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>( *clearValue );
    }

    auto heapProperty = CD3DX12_HEAP_PROPERTIES( D3D12_HEAP_TYPE_DEFAULT );

    D3D12MA::ALLOCATION_DESC allocationDesc = {};
    allocationDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    auto& allocator = App.Renderer().D3D12MA_Allocator();

    log::ThrowIfFailed(
        allocator.CreateResource(
            &allocationDesc,
            &resourceDesc,
            D3D12_RESOURCE_STATE_COMMON,
            m_D3D12ClearValue.get(),
            &m_Allocation,
            IID_PPV_ARGS(&m_D3D12Resource)
        )
    );

    // pre-d3d12ma way of creating a resource
    //log::ThrowIfFailed(
    //    device->CreateCommittedResource(
    //        &heapProperty,
    //        D3D12_HEAP_FLAG_NONE,
    //        &resourceDesc,
    //        D3D12_RESOURCE_STATE_COMMON,
    //        m_D3D12ClearValue.get(),
    //        IID_PPV_ARGS( &m_D3D12Resource )
    //    ) 
    //);

    ResourceStateTracker::AddGlobalResourceState(
        m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON
    );

    SetName( name );
}

Resource::Resource(ComPtr<ID3D12Resource> resource, const std::wstring& name)
    : m_D3D12Resource{ resource }
{
    SetName( name );
}

Resource::Resource(Resource&& copy)
    : m_D3D12Resource  { std::move( copy.m_D3D12Resource   ) }
    , m_ResourceName   { std::move( copy.m_ResourceName    ) }
    , m_D3D12ClearValue{ std::move( copy.m_D3D12ClearValue ) }
    , m_Allocation{ copy.m_Allocation }
{
    copy.m_Allocation = nullptr;
    copy.m_D3D12Resource = nullptr;
}

Resource& Resource::operator=(Resource&& other)
{
    if ( this != &other ) {
        Reset();
        m_Allocation = other.m_Allocation;
        m_D3D12Resource = other.m_D3D12Resource;
        m_ResourceName = other.m_ResourceName;
        m_D3D12ClearValue = std::move( other.m_D3D12ClearValue );

        other.m_D3D12Resource.Reset();
        other.m_ResourceName.clear();
        other.m_Allocation = nullptr;
    }

    return *this;
}

void Resource::SetD3D12Resource(
    ComPtr<ID3D12Resource> d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
    m_D3D12Resource = d3d12Resource;
    if ( clearValue ) {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>( *clearValue );
    }
    else {
        m_D3D12ClearValue.reset();
    }

    SetName( m_ResourceName );
}

void Resource::SetName(const std::wstring& name)
{
    m_ResourceName = name;
    if ( m_D3D12Resource && !m_ResourceName.empty() ) {
        m_D3D12Resource->SetName( m_ResourceName.c_str() );
    }
}

void Resource::Reset()
{
    if (m_Allocation)
    {
        m_Allocation->Release();
        m_Allocation = nullptr;
    }
    m_D3D12Resource.Reset();
    m_D3D12ClearValue.reset();
}

Resource::~Resource()
{
    Reset();
}
