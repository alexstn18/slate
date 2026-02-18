#pragma once
namespace slate {
    class Resource;

    class Texture : public Resource {
    public:
        Texture(const std::filesystem::path& path, const std::wstring& name);
    };
}

