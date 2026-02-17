#include "pch.hpp"
#include "rendering/model.hpp"
#include "rendering/mesh.hpp"
#include "rendering/command_list.hpp"
#include "rendering/command_queue.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using namespace slate;

Model::Model(const std::filesystem::path& filePath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath.string(), aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		log::Error("Assimp: {}", importer.GetErrorString());
		return; // Add this
	}

	log::Info("Loaded {} meshes", scene->mNumMeshes); // Add this
	ProcessNode(scene->mRootNode, scene);
	log::Info("Processed {} meshes total", m_Meshes.size()); // Add this
}

std::shared_ptr<Model> Model::Load(const std::filesystem::path& path, CommandList& commandList, CommandQueue& commandQueue, ComPtr<ID3D12CommandAllocator> allocator)
{
	auto model = std::make_shared<Model>(path);

	commandList.Reset(allocator);
	for (auto& mesh : model->GetMeshes())
	{
		mesh->CreateBuffers(commandList);
	}
	commandList.Close();

	uint64_t fence = commandQueue.ExecuteCommandLists({ commandList.Get().Get() });
	commandQueue.WaitForFenceValue(fence);

	return model;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (u32 i{ 0u }; i < node->mNumMeshes; ++i)
	{
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Meshes.push_back(ProcessMesh(mesh));
	}

	for (u32 i{ 0u }; i < node->mNumChildren; ++i)
	{
		ProcessNode(node->mChildren[i], scene);
	}
}

std::shared_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh/*, const aiScene* scene*/)
{
	std::vector<Vertex> vertices{};
	std::vector<u32> indices{};

	vertices.reserve(mesh->mNumVertices);

	for (u32 i{ 0u }; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex{};

		vertex.position = { mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z };
		vertex.color = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);

		vertices.push_back(vertex);
	}

	for (u32 i{ 0u }; i < mesh->mNumFaces; ++i)
	{
		const aiFace& face = mesh->mFaces[i];
		for (u32 j{ 0u }; j < face.mNumIndices; ++j)
		{
			indices.push_back(face.mIndices[j]);
		}
	}

	log::Info("Mesh has {} vertices, {} indices", vertices.size(), indices.size());
	return std::make_shared<Mesh>(vertices, indices);
}
