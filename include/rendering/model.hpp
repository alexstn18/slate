#pragma once

struct aiNode;
struct aiScene;
struct aiMesh;

namespace slate {
	class Mesh;
	class Texture;

	class Model {
	public:
		Model(const std::filesystem::path& filePath);

		static std::shared_ptr<Model> Load(const std::filesystem::path& path, CommandList& commandList, CommandQueue& commandQueue, ComPtr<ID3D12CommandAllocator> allocator);

		[[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const noexcept { return m_Meshes; }

	private:
		void ProcessNode(::aiNode* node, const ::aiScene* scene);
		std::shared_ptr<Mesh> ProcessMesh(::aiMesh* mesh/*, const ::aiScene* scene*/);

		std::vector<std::shared_ptr<Mesh>> m_Meshes{};
		std::vector<std::shared_ptr<Texture>> m_Textures{};
	};
}