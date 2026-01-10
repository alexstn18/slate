#include "pch.hpp"
#include "rendering/root_signature.hpp"

using namespace slate;

void RootSignature::Initialize(const D3D12_ROOT_SIGNATURE_DESC& desc)
{
}

uint32_t RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	u32 descriptorTableBitMask{ 0 };
	switch (type) {
		case D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV:
			descriptorTableBitMask = m_DescriptorTableBitMask;
			break;
		case D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER:
			descriptorTableBitMask = m_SamplerTableBitMask;
			break;
	}

	return descriptorTableBitMask;
}

uint32_t RootSignature::GetNumDescriptors(uint32_t rootIndex) const
{
	assert(rootIndex < 32);

	return m_NumDescriptorsPerTable[rootIndex];
}

void RootSignature::AnalyzeRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
{
}
