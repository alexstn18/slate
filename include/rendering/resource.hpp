#pragma once

#include <d3d12ma/D3D12MemAlloc.h>

// source:
// https://github.com/jpvanoosten/LearningDirectX12/blob/main/DX12Lib/src/Resource.cpp

namespace slate
{
	class Resource
	{
	public:
		Resource(const std::wstring& name = L"");
		Resource(const D3D12_RESOURCE_DESC& resourceDesc,
			const D3D12_CLEAR_VALUE* clearValue = nullptr,
			const std::wstring& name = L"");
		Resource(ComPtr<ID3D12Resource> resource, const std::wstring& name = L"");
		Resource(const Resource&) = delete;
		Resource(Resource&& copy);

		Resource& operator=(const Resource&) = delete;
		Resource& operator=(Resource&& other);

		bool IsValid() const
		{
			return (m_D3D12Resource != nullptr);
		}

		ComPtr<ID3D12Resource> D3D12Resource() const { return m_D3D12Resource; }
		D3D12_RESOURCE_DESC GetResourceDesc() const { return m_ResourceDesc; }

		// Replace the D3D12 resource
		// Should only be called by the CommandList.
		virtual void SetD3D12Resource(ComPtr<ID3D12Resource> d3d12Resource,
			const D3D12_CLEAR_VALUE* clearValue = nullptr);
		/**
		 * Get the SRV for a resource.
		 *
		 * @param srvDesc The description of the SRV to return. The default is nullptr
		 * which returns the default SRV for the resource (the SRV that is created when no
		 * description is provided.
		 */
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const = 0;

		/**
		 * Get the UAV for a (sub)resource.
		 *
		 * @param uavDesc The description of the UAV to return.
		 */
		virtual D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const = 0;

		void SetName(const std::wstring& name);
		const std::wstring& GetName() const { return m_ResourceName; }

		/**
		 * Release the underlying resource.
		 * This is useful for swap chain resizing.
		 */
		virtual void Reset();
	protected:
		virtual ~Resource();

		D3D12MA::Allocation* m_Allocation{ nullptr };
		ComPtr<ID3D12Resource> m_D3D12Resource{ nullptr };
		D3D12_FEATURE_DATA_FORMAT_SUPPORT m_FormatSupport{};
		std::unique_ptr<D3D12_CLEAR_VALUE> m_D3D12ClearValue{ nullptr };
		std::wstring m_ResourceName{};
		D3D12_RESOURCE_DESC m_ResourceDesc{};
	};
}

