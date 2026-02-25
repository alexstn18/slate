#include "pch.hpp"
#include "rendering/resource.hpp"
#include "rendering/texture.hpp"
#include "rendering/resource_state_tracker.hpp"
#include "rendering/descriptor_heap.hpp"

#include "assimp/texture.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"

using namespace slate;

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
    u8* textureBuffer = stbi_load(path.string().c_str(), &width, &height, &components, 4);

    if (!textureBuffer) {
        log::Critical("Could not load texture from path: {}", path.string());
    }

    finally f{ textureBuffer };

    Initialize(name, width, height, textureBuffer);

}

Texture::Texture(const aiTexture* embedded, const std::wstring& name)
{
    i32 width{}, height{}, components{};
    u8* textureBuffer{ nullptr };

    finally f;

    if (embedded->mHeight == 0) {
        textureBuffer = stbi_load_from_memory(
            reinterpret_cast<const stbi_uc*>(embedded->pcData),
            embedded->mWidth,
            &width, &height, &components, 4
        );
        f.ptr = textureBuffer;
    }
    else {
        width = embedded->mWidth;
        height = embedded->mHeight;
        textureBuffer = reinterpret_cast<u8*>(embedded->pcData);
    }

    Initialize(name, width, height, textureBuffer);
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC*) const
{
    return m_SRVHandle;
}

D3D12_CPU_DESCRIPTOR_HANDLE Texture::GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC*) const
{
    throw std::exception("Texture does not support UAV");
}

void Texture::Initialize(const std::wstring& name, int width, int height, void* textureBuffer )
{
    auto device = App.Renderer().D3D12Device();
    auto commandList = App.Renderer().D3D12CommandList();

    m_ResourceDesc.MipLevels = 1;
    // @TODO: add function for DXGI_FORMAT checking
    m_ResourceDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    m_ResourceDesc.Width = width;
    m_ResourceDesc.Height = height;
    m_ResourceDesc.DepthOrArraySize = 1;
    m_ResourceDesc.SampleDesc.Count = 1;
    m_ResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;

    CD3DX12_HEAP_PROPERTIES heapProps(D3D12_HEAP_TYPE_DEFAULT);

    log::ThrowIfFailed(device->CreateCommittedResource(
        &heapProps,
        D3D12_HEAP_FLAG_NONE,
        &m_ResourceDesc,
        D3D12_RESOURCE_STATE_COPY_DEST,
        nullptr,
        IID_PPV_ARGS(m_D3D12Resource.ReleaseAndGetAddressOf())
    ));

    UINT64 uploadSize{ 0ull };
    device->GetCopyableFootprints(
        &m_ResourceDesc,
        0, 1, 0,
        nullptr, nullptr, nullptr,
        &uploadSize
    );

    ComPtr<ID3D12Resource> uploadResource{ nullptr };
    CD3DX12_HEAP_PROPERTIES uploadHeap(D3D12_HEAP_TYPE_UPLOAD);
    auto uploadDesc = CD3DX12_RESOURCE_DESC::Buffer(uploadSize);
    log::ThrowIfFailed(device->CreateCommittedResource(
        &uploadHeap,
        D3D12_HEAP_FLAG_NONE,
        &uploadDesc,
        D3D12_RESOURCE_STATE_GENERIC_READ,
        nullptr,
        IID_PPV_ARGS(uploadResource.ReleaseAndGetAddressOf())
    ));

    D3D12_SUBRESOURCE_DATA textureData = {};
    textureData.pData = textureBuffer;
    textureData.RowPitch = width * 4;
    textureData.SlicePitch = height * width * 4;

    UpdateSubresources(
        commandList.Get(),
        m_D3D12Resource.Get(),
        uploadResource.Get(),
        0, 0, 1,
        &textureData
    );

    CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(
        m_D3D12Resource.Get(),
        D3D12_RESOURCE_STATE_COPY_DEST,
        D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE
    );

    commandList->ResourceBarrier(1, &barrier);

    ResourceStateTracker::AddGlobalResourceState(
        m_D3D12Resource.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);

    D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
    srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
    srvDesc.Format = m_ResourceDesc.Format;
    srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = m_ResourceDesc.MipLevels;

    auto& srvDscHeap = App.Renderer().GetSRVDescriptorHeap();

    u32 slot = App.Renderer().IncrementTextureCount();
    m_SRVHandle = srvDscHeap.GetCPUHandle(slot);
    m_GPUHandle = srvDscHeap.GetGPUHandle(slot);

    device->CreateShaderResourceView(
        m_D3D12Resource.Get(),
        &srvDesc,
        m_SRVHandle
    );

    SetName(name);

    App.Renderer().TrackUpload(std::move(uploadResource));
}
