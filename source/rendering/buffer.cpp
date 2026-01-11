#include "pch.hpp"
#include "rendering/buffer.hpp"

using namespace slate;

Buffer::Buffer(const D3D12_RESOURCE_DESC& resourceDesc)
	: Resource(resourceDesc)
{
}

Buffer::Buffer(ComPtr<ID3D12Resource> resource)
	: Resource(resource)
{
}
