#include "pch.hpp"
#include "rendering/resource.hpp"
#include "rendering/texture.hpp"
#include "rendering/resource_state_tracker.hpp"
#include "rendering/descriptor_heap.hpp"

#include "assimp/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

#include "DirectXTex.h"

using namespace slate;

// C/C++ does not have a finally block for exception handling
// so one needs to implement it themselves
// https://docs.oracle.com/javase/tutorial/essential/exceptions/finally.html
struct finally
{
    ~finally()
    {
        stbi_image_free(ptr);
    }

    void* ptr = nullptr;
};

Texture::Texture(const std::filesystem::path& path, const std::wstring& name)
{
    i32 width{}, height{}, components{};
    u8* textureBuffer = stbi_load(
        path.string().c_str(), &width, &height, &components, 4
    );

    if ( !textureBuffer ) {
        log::Critical( "Could not load texture from path: {}", path.string() );
    }

    finally f{ textureBuffer };

    Initialize( name, width, height, textureBuffer );

}

Texture::Texture(const aiTexture* embedded, const std::wstring& name)
{
    i32 width{}, height{}, components{};
    u8* textureBuffer{ nullptr };

    finally f;

    if ( embedded->mHeight == 0u ) {
        textureBuffer = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>( embedded->pcData ),
            embedded->mWidth,
            &width, &height, &components, 4
        );

        f.ptr = textureBuffer;
    }
    else {
        width = embedded->mWidth;
        height = embedded->mHeight;
        textureBuffer = reinterpret_cast<u8*>( embedded->pcData );
    }

    Initialize( name, width, height, textureBuffer );
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView(
    const D3D12_SHADER_RESOURCE_VIEW_DESC*) const
{
    return m_SRVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(
    const D3D12_UNORDERED_ACCESS_VIEW_DESC*) const
{
    throw std::exception( "Texture does not support UAV" );
}

void Texture::Initialize(
    const std::wstring& name, int width, int height, void* textureBuffer )
{
    auto device = App.Renderer().D3D12Device();
    auto commandList = App.Renderer().D3D12CommandList();
    auto& allocator = App.Renderer().D3D12MA_Allocator();

    DirectX::Image image{};
    DirectX::ScratchImage mipChain{};

    image.width = width;
    image.height = height;
    image.format = DXGI_FORMAT_R8G8B8A8_UNORM;
    image.rowPitch = width * 4;
    image.slicePitch = width * height * 4;
    image.pixels = reinterpret_cast<uint8_t*>( textureBuffer );

    log::ThrowIfFailed(
        DirectX::GenerateMipMaps(
            image,
            DirectX::TEX_FILTER_DEFAULT,
            0, // 0 = full mipchain
            mipChain
        )
    );

    const DirectX::TexMetadata& metadata{ mipChain.GetMetadata() };
    UINT mipLevels{ static_cast<UINT>( metadata.mipLevels ) };

    m_ResourceDesc.MipLevels = static_cast<UINT16>( mipLevels );
    // @TODO: add function for DXGI_FORMAT checking
    m_ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_ResourceDesc.Width = width;
    m_ResourceDesc.Height = height;
    m_ResourceDesc.DepthOrArraySize = 1;
    m_ResourceDesc.SampleDesc.Count = 1;
    m_ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    D3D12MA::ALLOCATION_DESC allocDesc = {};
    allocDesc.HeapType = D3D12_HEAP_TYPE_DEFAULT;

    log::ThrowIfFailed(
        allocator.CreateResource(
            &allocDesc,
            &m_ResourceDesc,
            D3D12_RESOURCE_STATE_COPY_DEST,
            nullptr,
            &m_Allocation,
            IID_PPV_ARGS( m_D3D12Resource.ReleaseAndGetAddressOf() )
        )
    );

    // subresource data for available mip levels
    UINT subResourceCount{ mipLevels };
    std::vector<D3D12_SUBRESOURCE_DATA> subResources{ subResourceCount };

    for ( UINT i{ 0u }; i < subResourceCount; ++i ) {
        const DirectX::Image* img = mipChain.GetImage( i, 0ull, 0ull );
        subResources[ i ].pData      = img->pixels;
        subResources[ i ].RowPitch   = static_cast<LONG_PTR>( img->rowPitch );
        subResources[ i ].SlicePitch = static_cast<LONG_PTR>( img->slicePitch );
    }

    UINT64 uploadSize{ 
        GetRequiredIntermediateSize( m_D3D12Resource.Get(), 0u, subResourceCount ) 
    };

    D3D12MA::ALLOCATION_DESC uploadAllocDesc = {};
    uploadAllocDesc.HeapType = D3D12_HEAP_TYPE_UPLOAD;
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer( uploadSize );

    D3D12MA::Allocation* uploadAllocation{ nullptr };
    ComPtr<ID3D12Resource> uploadResource{ nullptr };
    
    log::ThrowIfFailed(
        allocator.CreateResource(
            &uploadAllocDesc,
            &uploadDesc,
            D3D12_RESOURCE_STATE_GENERIC_READ,
            nullptr,
            &uploadAllocation,
            IID_PPV_ARGS( uploadResource.ReleaseAndGetAddressOf() )
        )
    );

    UpdateSubresources(
        commandList.Get(),
        m_D3D12Resource.Get(),
        uploadResource.Get(),
        0, 0, subResourceCount,
        subResources.data()
    );

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_D3D12Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->ResourceBarrier( 1, &barrier );

    ResourceStateTracker::AddGlobalResourceState(
        m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = m_ResourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mipLevels;

    auto& srvDscHeap = App.Renderer().GetSRVDescriptorHeap();

    u32 slot = srvDscHeap.GetNextIndex();
    m_SRVHandle = srvDscHeap.GetCPUHandle(slot);
    m_GPUHandle = srvDscHeap.GetGPUHandle(slot);

    device->CreateShaderResourceView(
        m_D3D12Resource.Get(),
        &srvDesc,
        m_SRVHandle
    );

    SetName( name );

    App.Renderer().TrackUpload( std::move( uploadResource ), uploadAllocation );
}