#pragma once

namespace slate
{
	// Helper class generated with Claude AI
	class RootSignature
	{
	public:
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;

		RootSignature& AddRootCBV(u32 shaderRegister);
		RootSignature& AddRootConstants(u32 shaderRegister, u32 num32BitValues);
		RootSignature& AddDescriptorTable();
		RootSignature& AddSRVs(u32 baseRegister, u32 count);
		RootSignature& AddUAVs(u32 baseRegister, u32 count);

		void Initialize();

		ComPtr<ID3D12RootSignature> Get() const { return m_RootSignature; }
		ID3D12RootSignature* operator->() const { return m_RootSignature.Get(); }

		uint32_t GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
		uint32_t GetNumDescriptors(uint32_t rootIndex) const;
	private:
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

		struct Table {
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		};

		std::vector<D3D12_ROOT_PARAMETER> m_Params{};
		std::vector<Table> m_Tables{};
	};
}