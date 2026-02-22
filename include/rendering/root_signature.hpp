#pragma once

namespace slate
{
	// Helper class combined with 
	// https://github.com/jpvanoosten/LearningDirectX12/blob/main/DX12Lib/src/RootSignature.cpp
	// and some parts of it generated with Claude AI
	class RootSignature
	{
	public:
		RootSignature() = default;
		virtual ~RootSignature() = default;
		RootSignature(const RootSignature&) = delete;
		RootSignature& operator=(const RootSignature&) = delete;

		RootSignature& AddRootCBV(u32 shaderRegister);
		RootSignature& AddRootConstants(u32 shaderRegister, u32 num32BitValues);
		RootSignature& AddDescriptorTable();
		RootSignature& AddSRVs(u32 baseRegister, u32 count);
		RootSignature& AddUAVs(u32 baseRegister, u32 count);
		RootSignature& AddStaticSampler(u32 shaderRegister);

		void Initialize();

		[[nodiscard]] ComPtr<ID3D12RootSignature> Get() const noexcept { return m_RootSignature; }
		ID3D12RootSignature* operator->() const { return m_RootSignature.Get(); }

		[[nodiscard]] u32 GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept;
		[[nodiscard]] u32 GetNumDescriptors(uint32_t rootIndex) const noexcept;
		[[nodiscard]] D3D12_ROOT_SIGNATURE_DESC GetRootSignatureDesc() const noexcept { return m_RootSignatureDesc; }
	private:
		ComPtr<ID3D12RootSignature> m_RootSignature{ nullptr };

		D3D12_ROOT_SIGNATURE_DESC m_RootSignatureDesc{};

		// Need to know the number of descriptors per descriptor table
		// A maximum of 32-bit descriptor tables are supported
		// (since a 32-bit mask is used to represent the descriptor
		// tables in the root signature)
		u32 m_NumDescriptorsPerTable[32]{};
		
		// A bit mask that represents the root parameter indices
		// for samplers
		u32 m_SamplerTableBitMask{};

		// A bit mask that represents the root parameter indices 
		// that are CBV, UAV and SRV descriptor tables
		u32 m_DescriptorTableBitMask{};

		struct Table {
			std::vector<D3D12_DESCRIPTOR_RANGE> ranges;
		};

		std::vector<D3D12_ROOT_PARAMETER> m_Params{};
		std::vector<D3D12_STATIC_SAMPLER_DESC> m_StaticSamplers{};
		std::vector<Table> m_Tables{};
	};
}