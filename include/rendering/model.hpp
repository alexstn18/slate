#pragma once

namespace slate {
	class Mesh;
	class Texture;

	class Model {
	public:
		Model(const std::filesystem::path& filePath);

		static std::shared_ptr<Model> Load(const std::filesystem::path& path, CommandList& commandList, CommandQueue& commandQueue, ComPtr<ID3D12CommandAllocator> allocator);

		void AddMesh(std::shared_ptr<Mesh> mesh);

		[[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const noexcept { return m_Meshes; }

	private:
		std::vector<std::shared_ptr<Mesh>> m_Meshes{};
	};
}