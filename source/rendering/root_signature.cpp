#include "pch.hpp"
#include "rendering/root_signature.hpp"

using namespace slate;


void RootSignature::Initialize()
{
    auto device = App.Renderer().D3D12Device();

    // Reset bitmasks
    m_DescriptorTableBitMask = 0;
    m_SamplerTableBitMask = 0;
    memset( m_NumDescriptorsPerTable, 0, sizeof( m_NumDescriptorsPerTable ) );

    size_t tableIdx{ 0ull };
    for ( size_t i{ 0ull }; i < m_Params.size(); ++i)
    {
        auto& param = m_Params[ i ];

        if ( param.ParameterType == D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE )
        {
            auto& ranges = m_Tables[ tableIdx ].ranges;
            param.DescriptorTable.NumDescriptorRanges = ( UINT )ranges.size();
            param.DescriptorTable.pDescriptorRanges = ranges.data();

            // Count total descriptors in this table
            u32 numDescriptors = 0u;
            bool isSamplerTable = false;

            for ( const auto& range : ranges )
            {
                numDescriptors += range.NumDescriptors;

                // Check if this is a sampler table
                if ( range.RangeType == D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER )
                {
                    isSamplerTable = true;
                }
            }

            m_NumDescriptorsPerTable[ i ] = numDescriptors;

            // Set bit in appropriate bitmask
            if ( isSamplerTable )
            {
                m_SamplerTableBitMask |= ( 1 << i );
            }
            else
            {
                m_DescriptorTableBitMask |= ( 1 << i );
            }

            tableIdx++;
        }
    }

    // Store the descriptor for later queries
    m_RootSignatureDesc.NumParameters = ( UINT )m_Params.size();
    m_RootSignatureDesc.pParameters = m_Params.data();
    m_RootSignatureDesc.NumStaticSamplers = ( UINT )m_StaticSamplers.size();
    m_RootSignatureDesc.pStaticSamplers = m_StaticSamplers.data();
    m_RootSignatureDesc.Flags = 
        D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

    ComPtr<ID3DBlob> blob{ nullptr };
    ComPtr<ID3DBlob> errorBlob{ nullptr };
    D3D12SerializeRootSignature(
        &m_RootSignatureDesc, D3D_ROOT_SIGNATURE_VERSION_1, &blob, &errorBlob
    );

    if (errorBlob) {
        log::Critical("{}", (char*)errorBlob->GetBufferPointer());
    }

    device->CreateRootSignature( 0u, blob->GetBufferPointer(), blob->GetBufferSize(),
        IID_PPV_ARGS( &m_RootSignature ) );
}

[[nodiscard]] uint32_t RootSignature::GetDescriptorTableBitMask(
    D3D12_DESCRIPTOR_HEAP_TYPE type) const noexcept
{
	u32 descriptorTableBitMask{ 0u };
	switch ( type ) {
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
	assert( rootIndex < 32 );

	return m_NumDescriptorsPerTable[ rootIndex ];
}

RootSignature& RootSignature::AddRootCBV(uint32_t shaderRegister)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    param.Descriptor.ShaderRegister = shaderRegister;

    m_Params.push_back( param );
    return *this;
}

RootSignature& RootSignature::AddRootConstants(
    uint32_t shaderRegister, uint32_t num32BitValues)
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
    param.Constants.ShaderRegister = shaderRegister;
    param.Constants.Num32BitValues = num32BitValues;

    m_Params.push_back( param );
    return *this;
}

RootSignature& RootSignature::AddCBVs(u32 baseRegister, u32 count)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
    range.NumDescriptors = count;
    range.BaseShaderRegister = baseRegister;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
    m_Tables.back().ranges.push_back( range );
    return *this;
}

RootSignature& RootSignature::AddDescriptorTable()
{
    D3D12_ROOT_PARAMETER param = {};
    param.ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
    param.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

    m_Params.push_back( param );
    m_Tables.push_back( Table{} );
    return *this;
}

RootSignature& RootSignature::AddSRVs(uint32_t baseRegister, uint32_t count)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
    range.NumDescriptors = count;
    range.BaseShaderRegister = baseRegister;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    m_Tables.back().ranges.push_back( range );
    return *this;
}

RootSignature& RootSignature::AddUAVs(uint32_t baseRegister, uint32_t count)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_UAV;
    range.NumDescriptors = count;
    range.BaseShaderRegister = baseRegister;
    range.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

    m_Tables.back().ranges.push_back( range );
    return *this;
}

RootSignature& RootSignature::AddStaticSampler(u32 shaderRegister)
{
    D3D12_STATIC_SAMPLER_DESC sampler = {};
    
    sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
    sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
    sampler.ShaderRegister = shaderRegister;
    sampler.RegisterSpace = 0;
    sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
    sampler.MinLOD = 0.0f;
    sampler.MaxLOD = D3D12_FLOAT32_MAX;

    m_StaticSamplers.push_back( sampler );
    return *this;
}