#include "pch.hpp"
#include "rendering/buffer.hpp"

using namespace slate;

D3D12_CPU_DESCRIPTOR_HANDLE slate::Buffer::GetShaderResourceView(
	const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc) const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

D3D12_CPU_DESCRIPTOR_HANDLE slate::Buffer::GetUnorderedAccessView(
	const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc) const
{
	return D3D12_CPU_DESCRIPTOR_HANDLE();
}

Buffer::Buffer(const D3D12_RESOURCE_DESC& resourceDesc)
	: Resource{ resourceDesc }
{
}

Buffer::Buffer(ComPtr<ID3D12Resource> resource)
	: Resource{ resource }
{
}
