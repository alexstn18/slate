#pragma once
#include "rendering/resource.hpp"

struct aiTexture;

namespace slate {
    class Texture : public Resource {
    public:
        Texture(const std::filesystem::path& path, const std::wstring& name);
        Texture(const aiTexture* embedded, const std::wstring& name);
        D3D12_CPU_DESCRIPTOR_HANDLE GetShaderResourceView(const D3D12_SHADER_RESOURCE_VIEW_DESC* srvDesc = nullptr) const override;
        D3D12_CPU_DESCRIPTOR_HANDLE GetUnorderedAccessView(const D3D12_UNORDERED_ACCESS_VIEW_DESC* uavDesc = nullptr) const override;
        [[nodiscard]] D3D12_GPU_DESCRIPTOR_HANDLE GetGPUHandle() const noexcept { return m_GPUHandle; }
    private:
        void Initialize(const std::wstring& name, int width, int height, void* textureBuffer, bool freeBuffer);

        D3D12_CPU_DESCRIPTOR_HANDLE m_SRVHandle{};
        D3D12_GPU_DESCRIPTOR_HANDLE m_GPUHandle{};
    };
}

