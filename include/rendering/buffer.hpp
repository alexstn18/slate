#pragma once

#include "rendering/resource.hpp"

namespace slate
{
	class Buffer : public Resource
	{
	public:
		D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;

		D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;
	protected:
		Buffer(const D3D12_RESOURCE_DESC& resourceDesc);
		Buffer(ComPtr<ID3D12Resource> resource);
	};
}