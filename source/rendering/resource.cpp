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
    auto device = App.Device().GetDevice();

    if (clearValue)
    {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }

    log::ThrowIfFailed(device->CreateCommittedResource(
        &CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
        D3D12_HEAP_FLAG_NONE,
        &resourceDesc,
        D3D12_RESOURCE_STATE_COMMON,
        m_D3D12ClearValue.get(),
        IID_PPV_ARGS(&m_D3D12Resource)
    ));

    ResourceStateTracker::AddGlobalResourceState(m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_COMMON);

    SetName(name);
}

Resource::Resource(Microsoft::WRL::ComPtr<ID3D12Resource> resource, const std::wstring& name)
    : m_D3D12Resource(resource)
{
    SetName(name);
}

Resource::Resource(const Resource& copy)
    : m_D3D12Resource(copy.m_D3D12Resource)
    , m_ResourceName(copy.m_ResourceName)
    , m_D3D12ClearValue(std::make_unique<D3D12_CLEAR_VALUE>(*copy.m_D3D12ClearValue))
{
}

Resource::Resource(Resource&& copy)
    : m_D3D12Resource(std::move(copy.m_D3D12Resource))
    , m_ResourceName(std::move(copy.m_ResourceName))
    , m_D3D12ClearValue(std::move(copy.m_D3D12ClearValue))
{
}

Resource& Resource::operator=(const Resource& other)
{
    if (this != &other)
    {
        m_D3D12Resource = other.m_D3D12Resource;
        m_ResourceName = other.m_ResourceName;
        if (other.m_D3D12ClearValue)
        {
            m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*other.m_D3D12ClearValue);
        }
    }

    return *this;
}

Resource& Resource::operator=(Resource&& other)
{
    if (this != &other)
    {
        m_D3D12Resource = other.m_D3D12Resource;
        m_ResourceName = other.m_ResourceName;
        m_D3D12ClearValue = std::move(other.m_D3D12ClearValue);

        other.m_D3D12Resource.Reset();
        other.m_ResourceName.clear();
    }

    return *this;
}


Resource::~Resource()
{
}

void Resource::SetD3D12Resource(Microsoft::WRL::ComPtr<ID3D12Resource> d3d12Resource, const D3D12_CLEAR_VALUE* clearValue)
{
    m_D3D12Resource = d3d12Resource;
    if (m_D3D12ClearValue)
    {
        m_D3D12ClearValue = std::make_unique<D3D12_CLEAR_VALUE>(*clearValue);
    }
    else
    {
        m_D3D12ClearValue.reset();
    }
    SetName(m_ResourceName);
}

void Resource::SetName(const std::wstring& name)
{
    m_ResourceName = name;
    if (m_D3D12Resource && !m_ResourceName.empty())
    {
        m_D3D12Resource->SetName(m_ResourceName.c_str());
    }
}

void Resource::Reset()
{
    m_D3D12Resource.Reset();
    m_D3D12ClearValue.reset();
}