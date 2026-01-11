#include "pch.hpp"
#include "rendering/root_signature.hpp"

using namespace slate;

void RootSignature::Initialize()
{
    auto device = App.Renderer().D3D12Device();

    size_t tableIdx = 0;
    for (auto& param : m_Params)
    {
        if (param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE)
        {
            auto& ranges = m_Tables[tableIdx].ranges;
            param.DescriptorTable.NumDescriptorRanges = (UINT)ranges.size();
            param.DescriptorTable.pDescriptorRanges = ranges.data();
            tableIdx++;
        }
    }

    D3D12_ROOT_SIGNATURE_DESC desc = {};
    desc.NumParameters = (UINT)m_Params.size();
    desc.pParameters = m_Params.data();
    desc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob;
    D3D12SerializeRootSignature(&desc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, nullptr);
    device->CreateRootSignature(0, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS(&m_RootSignature));
}

[[nodiscard]] uint32_t RootSignature::GetDescriptorTableBitMask(D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
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

[[nodiscard]] uint32_t RootSignature::GetNumDescriptors(uint32_t rootIndex) const noexcept
{
	assert(rootIndex < 32);

	return m_NumDescriptorsPerTable[rootIndex];
}

RootSignature& RootSignature::AddRootCBV(uint32_t shaderRegister)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    param.Descriptor.ShaderRegister = shaderRegister;

    m_Params.push_back(param);
    return *this;
}

RootSignature& RootSignature::AddRootConstants(uint32_t shaderRegister, uint32_t num32BitValues)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.Num32BitValues = num32BitValues;

    m_Params.push_back(param);
    return *this;
}

RootSignature& RootSignature::AddDescriptorTable()
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    m_Params.push_back(param);
    m_Tables.push_back(Table{});
    return *this;
}

RootSignature& RootSignature::AddSRVs(uint32_t baseRegister, uint32_t count)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = count;
    range.BaseShaderRegister = baseRegister;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    m_Tables.back().ranges.push_back(range);
    return *this;
}

RootSignature& RootSignature::AddUAVs(uint32_t baseRegister, uint32_t count)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = count;
    range.BaseShaderRegister = baseRegister;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    m_Tables.back().ranges.push_back(range);
    return *this;
}
