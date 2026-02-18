#include "pch.hpp"
#include "rendering/model.hpp"
#include "rendering/mesh.hpp"
#include "rendering/texture.hpp"
#include "rendering/render_components.hpp"
#include "rendering/command_list.hpp"
#include "rendering/command_queue.hpp"

#include "assimp/Importer.hpp"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

using namespace slate;

Model::Model(const std::filesystem::path& filePath)
{
	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(filePath.string(), 
		aiProcess_Triangulate | aiProcess_JoinIdenticalVertices);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || 
		!scene->mRootNode) {
		log::Error("Assimp: {}", importer.GetErrorString());
		return; // Add this
	}

	log::Info("Loaded {} meshes", scene->mNumMeshes);
	ProcessNode(scene->mRootNode, scene);
	log::Info("Processed {} meshes total", m_Meshes.size());
}

std::shared_ptr<Model> Model::Load(const std::filesystem::path& path, 
	CommandList& commandList, CommandQueue& commandQueue, 
	ComPtr<ID3D12CommandAllocator> allocator)
{
	commandList.Reset(allocator);
	
	auto model = std::make_shared<Model>(path);

	for (auto& mesh : model->GetMeshes()) {
		mesh->CreateBuffers(commandList);
	}
	commandList.Close();

	uint64_t fence = commandQueue.ExecuteCommandLists(
		{ commandList.Get().Get() }
	);
	commandQueue.WaitForFenceValue(fence);

	App.Renderer().FlushUploads();

	return model;
}

void Model::ProcessNode(aiNode* node, const aiScene* scene)
{
	for (u32 i{ 0u }; i < node->mNumMeshes; ++i) {
		aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
		m_Meshes.push_back(ProcessMesh(mesh, scene));
	}

	for (u32 i{ 0u }; i < node->mNumChildren; ++i) {
		ProcessNode(node->mChildren[i], scene);
	}
}

std::shared_ptr<Mesh> Model::ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
	std::vector<Vertex> vertices{};
	std::vector<u32> indices{};
	Material material{};

	vertices.reserve(mesh->mNumVertices);

	for (u32 i{ 0u }; i < mesh->mNumVertices; ++i)
	{
		Vertex vertex{};

		vertex.position = { 
			mesh->mVertices[i].x,
			mesh->mVertices[i].y,
			mesh->mVertices[i].z 
		};

		if (mesh->HasNormals()) {
			vertex.normal = { 
				mesh->mNormals[i].x, 
				mesh->mNormals[i].y,
				mesh->mNormals[i].z 
			};
		}

		if (mesh->HasTextureCoords(0)) {
			vertex.texCoords = {
				mesh->mTextureCoords[0][i].x,
				mesh->mTextureCoords[0][i].y
			};

			if (mesh->mTangents) {
				// tangent
				vertex.tangent = {
					mesh->mTangents[i].x,
					mesh->mTangents[i].y,
					mesh->mTangents[i].z
				};

				// bi-tangent
				vertex.bitangent = {
						mesh->mBitangents[i].x,
						mesh->mBitangents[i].y,
						mesh->mBitangents[i].z
				};
			} 
			else {
				// @TODO: mikktspace tangent calculation here
			}

		}
		else {
			vertex.texCoords = glm::vec2(0.0f, 0.0f);
		}

		vertices.push_back(vertex);
	}

	for (u32 i{ 0u }; i < mesh->mNumFaces; ++i) {
		const aiFace& face = mesh->mFaces[i];
		for (u32 j{ 0u }; j < face.mNumIndices; ++j) {
			indices.push_back(face.mIndices[j]);
		}
	}

	log::Info("Mesh has {} vertices, {} indices", 
		vertices.size(), indices.size());

	if (mesh->mMaterialIndex >= 0) {
		aiMaterial* _material = scene->mMaterials[mesh->mMaterialIndex];
		material.Albedo = LoadMaterialTexture(_material, aiTextureType_DIFFUSE, scene);
		material.Normal = LoadMaterialTexture(_material, aiTextureType_NORMALS, scene);
		material.OcclusionRoughnessMetallic = LoadMaterialTexture(_material, aiTextureType_LIGHTMAP, scene); // glTF PBR metallic-roughness
		material.Emissive = LoadMaterialTexture(_material, aiTextureType_EMISSIVE, scene);

		aiColor4D color{};
		if (_material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
			material.albedoFactor = { color.r, color.g, color.b, color.a };
		}

		_material->Get(AI_MATKEY_METALLIC_FACTOR, material.metallicFactor);
		_material->Get(AI_MATKEY_ROUGHNESS_FACTOR, material.roughnessFactor);
		
		aiColor3D emissive{};
		if (_material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
			material.emissiveFactor = { emissive.r, emissive.g, emissive.b };
		}
	}

	return std::make_shared<Mesh>(vertices, indices, material);
}

std::shared_ptr<Texture> Model::LoadMaterialTexture(const::aiMaterial* mat, aiTextureType type, const::aiScene* scene)
{
	if (mat->GetTextureCount(type) == 0) return nullptr;

	aiString str{};
	mat->GetTexture(type, 0, &str);
	const char* cStr = str.C_Str();

	auto it = m_TextureCache.find(cStr);
	if (it != m_TextureCache.end()) return it->second;

	std::shared_ptr<Texture> texture{ nullptr };

	if (const aiTexture* embedded = scene->GetEmbeddedTexture(cStr)) {
		texture = std::make_shared<Texture>(embedded, std::wstring(cStr, cStr + str.length));
	}
	else {
		texture = std::make_shared<Texture>(cStr,
			std::wstring(cStr, cStr + str.length));
	}

	m_TextureCache[cStr] = texture;

	return texture;
}
