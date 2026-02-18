#pragma once
namespace slate {
	class VertexBuffer;
	class IndexBuffer;

	struct Vertex {
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec2 texCoords;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	class Mesh {
	public:
		Mesh(const std::vector<Vertex>& vertices,
			 const std::vector<u32>& indices);
		void CreateBuffers(CommandList& commandList);

		[[nodiscard]] std::shared_ptr<VertexBuffer> GetVertexBuffer() const noexcept { return m_VertexBuffer; }
		[[nodiscard]] std::shared_ptr<IndexBuffer> GetIndexBuffer() const noexcept { return m_IndexBuffer; }
	
		[[nodiscard]] size_t GetVertexCount() const noexcept { return m_Vertices.size(); }
		[[nodiscard]] size_t GetIndexCount() const noexcept { return m_Indices.size(); }
	private:
		std::vector<Vertex> m_Vertices{};
		std::vector<u32> m_Indices{};

		std::shared_ptr<VertexBuffer> m_VertexBuffer{ nullptr };
		std::shared_ptr<IndexBuffer> m_IndexBuffer{ nullptr };
	};
}