#include "pch.hpp"
#include "rendering/resource.hpp"

using namespace slate;

D3D12_RESOURCE_DESC Resource::GetResourceDesc() const
{
	return D3D12_RESOURCE_DESC();
}

void Resource::SetName(const std::wstring& name)
{
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT1 formatSupport) const
{
	return false;
}

bool Resource::CheckFormatSupport(D3D12_FORMAT_SUPPORT2 formatSupport) const
{
	return false;
}

Resource::Resource(const D3D12_RESOURCE_DESC& resourceDesc, const D3D12_CLEAR_VALUE* clearValue)
{
}

Resource::Resource(ComPtr<ID3D12Resource> resource, const D3D12_CLEAR_VALUE* clearValue)
{
}

void Resource::CheckFeatureSupport()
{
}
