#pragma once
struct aiNode;
struct aiScene;
struct aiMesh;

namespace slate {
	class Mesh;

	class Model {
	public:
		Model(const std::filesystem::path& filePath);

		[[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const noexcept { return m_Meshes; }

	private:
		void ProcessNode(::aiNode* node, const ::aiScene* scene);
		std::shared_ptr<Mesh> ProcessMesh(::aiMesh* mesh/*, const ::aiScene* scene*/);

		std::vector<std::shared_ptr<Mesh>> m_Meshes{};
	};
}

