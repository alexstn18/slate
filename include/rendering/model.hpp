#pragma once

#include <glm/glm.hpp>

#include "rendering/render_components.hpp"

namespace slate {
	class Mesh;
	class Texture;
	class ConstantBuffer;

	void ClearTextureCache();

	class Model {
	public:
		Model(const std::filesystem::path& filePath);

		static std::shared_ptr<Model> Load(const std::filesystem::path& path, CommandList& commandList, CommandQueue& commandQueue, ComPtr<ID3D12CommandAllocator> allocator);

		void AddMesh(std::shared_ptr<Mesh> mesh);

		void Update(const glm::mat4& VP, const glm::vec3& cameraPos);

		[[nodiscard]] const std::vector<std::shared_ptr<Mesh>>& GetMeshes() const noexcept { return m_Meshes; }
		[[nodiscard]] const ConstantBuffer* GetConstantBuffer() const noexcept { return m_ConstantBuffer.get(); }
		[[nodiscard]] Transform& GetTransform() noexcept { return m_Transform; }
	private:
		std::vector<std::shared_ptr<Mesh>> m_Meshes{};

		std::unique_ptr<ConstantBuffer> m_ConstantBuffer{ nullptr };
	
		Transform m_Transform{};
		ModelConstants m_ModelConstants{};
	};
}