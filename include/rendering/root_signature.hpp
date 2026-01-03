#pragma once

namespace slate
{
	class RootSignature
	{
	public:
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;

		void Initialize(const D3D12_ROOT_SIGNATURE_DESC& desc);

		ComPtr<ID3D12RootSignature> Get() const { return m_RootSignature; }
		ID3D12RootSignature* operator->() const { return m_RootSignature.Get(); }

		uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		uint32_t GetNumDescriptors(uint32_t rootIndex) const;
	private:
		void AnalyzeRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc);

		ComPtr<ID3D12RootSignature> m_RootSignature{ nullptr };

		// Need to know the number of descriptors per descriptor table
		// A maximum of 32-bit descriptor tables are supported
		// (since a 32-bit mask is used to represent the descriptor
		// tables in the root signature)
		uint32_t m_NumDescriptorsPerTable[32]{};
		
		// A bit mask that represents the root parameter indices
		// for samplers
		uint32_t m_SamplerTableBitMask{};

		// A bit mask that represents the root parameter indices 
		// that are CBV, UAV and SRV descriptor tables
		uint32_t m_DescriptorTableBitMask{};
	};
}