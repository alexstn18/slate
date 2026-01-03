#include "pch.hpp"
#include "rendering/root_signature.hpp"

using namespace slate;

void RootSignature::Initialize(const D3D12_ROOT_SIGNATURE_DESC& desc)
{

}

uint32_t RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const
{
	return 0;
}

uint32_t RootSignature::GetNumDescriptors(uint32_t rootIndex) const
{
	return 0;
}

void RootSignature::AnalyzeRootSignature(const D3D12_VERSIONED_ROOT_SIGNATURE_DESC& desc)
{
}
