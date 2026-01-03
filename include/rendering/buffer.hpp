#pragma once

#include "rendering/resource.hpp"

namespace slate
{
	class Buffer : public Resource
	{
	protected:
		Buffer(const D3D12_RESOURCE_DESC& resourceDesc);
		Buffer(ComPtr<ID3D12Resource> resource);
	};
}