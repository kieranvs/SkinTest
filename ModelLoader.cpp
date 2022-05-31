
#include "ModelLoader.h"

#include "VulkanWrapper/Log.h"
#include "VulkanWrapper/Image.h"
#include "VulkanWrapper/Buffer.h"

#include "assimp/Importer.hpp"
#include "assimp/postprocess.h"
#include "assimp/scene.h"

inline glm::vec3 transform_by_matrix(const glm::vec3& v, const glm::mat4& m, bool is_position = true)
{
	return glm::vec3(m * glm::vec4(v, is_position ? 1.0 : 0.0));
}

void addMesh(aiMesh* mesh, std::vector<Vertex>& verts, const glm::mat4& bake_transform)
{
	std::vector<unsigned int> indices;
	for (unsigned int i = 0; i < mesh->mNumFaces; i++)
	{
		const auto& face = mesh->mFaces[i];
		for (unsigned int j = 0; j < face.mNumIndices; j++)
			indices.push_back(face.mIndices[j]);
	}

	for (auto x = 0; x < indices.size(); x++)
	{
		int i = indices[x];

		Vertex vert{};

		vert.pos = glm::vec3(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);

		vert.texCoord = glm::vec2(0.0f, 0.0f);
		if (mesh->mTextureCoords[0])
			vert.texCoord = glm::vec2(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y);

		vert.pos = transform_by_matrix(vert.pos, bake_transform, true);

		verts.push_back(vert);
	}
}

void loadModel(const std::string& file_path, const glm::mat4& bake_transform, std::vector<Mesh>& meshes, DeviceManager& device_manager)
{
	auto index = file_path.find_last_of("/\\");
	std::string texture_directory = file_path.substr(0, index + 1);

	Assimp::Importer importer;
	const aiScene* scene = importer.ReadFile(file_path, aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_GenSmoothNormals);

	if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
	{
		log_error(importer.GetErrorString());
		return;
	}

	for (unsigned int i = 0; i < scene->mNumMeshes; i++)
	{
		std::vector<Vertex> verts;
		addMesh(scene->mMeshes[i], verts, bake_transform);
		std::string tex_path;
		if (verts.empty()) continue;

		aiMaterial* material = scene->mMaterials[scene->mMeshes[i]->mMaterialIndex];
		if (material->GetTextureCount(aiTextureType_DIFFUSE) == 1)
		{
			aiString str;
			material->GetTexture(aiTextureType_DIFFUSE, 0, &str);
			tex_path = texture_directory + std::string(str.C_Str());
		}
		
		auto& mesh = meshes.emplace_back();
		mesh.texture.init(device_manager, tex_path);

	    std::vector<uint32_t> indices{};
	    indices.resize(verts.size());
	    for (int i = 0; i < verts.size(); ++i)
	    {
	        indices[i] = i;
	    }

	    uploadBufferData(device_manager, mesh.vertex_buffer, verts, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
	    uploadBufferData(device_manager, mesh.index_buffer, indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
	}

}
