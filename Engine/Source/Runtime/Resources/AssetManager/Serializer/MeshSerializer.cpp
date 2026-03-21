#include "Resources/AssetManager/Serializer/MeshSerializer.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <meshoptimizer.h>

namespace Yogi
{

Owner<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.Position = Vector3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal   = Vector3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        if (mesh->HasTextureCoords(0))
        {
            vertex.Texcoord = Vector2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
        }
        vertices.push_back(vertex);
    }
    for (int i = 0; i < mesh->mNumFaces; ++i)
    {
        const aiFace& Face = mesh->mFaces[i];
        YG_ASSERT(Face.mNumIndices == 3, "Load mesh error!");
        indices.push_back(Face.mIndices[0]);
        indices.push_back(Face.mIndices[1]);
        indices.push_back(Face.mIndices[2]);
    }

    // Optimize mesh using meshoptimizer
    std::vector<uint32_t> remap(vertices.size());
    size_t                uniqueVertices = meshopt_generateVertexRemap(
        remap.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));
    meshopt_remapVertexBuffer(vertices.data(), vertices.data(), vertices.size(), sizeof(Vertex), remap.data());
    meshopt_remapIndexBuffer(indices.data(), 0, indices.size(), remap.data());

    vertices.resize(uniqueVertices);
    meshopt_optimizeVertexCache(indices.data(), indices.data(), indices.size(), vertices.size());
    meshopt_optimizeVertexFetch(
        vertices.data(), indices.data(), indices.size(), vertices.data(), vertices.size(), sizeof(Vertex));

    return Owner<Mesh>::Create(vertices, indices);
}
Owner<Mesh> ProcessNode(aiNode* node, const aiScene* scene, const std::string& meshName)
{
    for (int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        if (meshName == mesh->mName.C_Str())
        {
            return ProcessMesh(mesh, scene);
        }
    }
    for (int i = 0; i < node->mNumChildren; ++i)
    {
        auto mesh = ProcessNode(node->mChildren[i], scene, meshName);
        if (mesh)
        {
            return mesh;
        }
    }
    return nullptr;
}

// --------------------------------------------------------------------------------

Owner<Mesh> MeshSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
{
    size_t      sepPos   = key.find("::");
    std::string filepath = key;
    std::string meshName = "";
    if (sepPos != std::string::npos)
    {
        filepath = key.substr(0, sepPos);
        meshName = key.substr(sepPos + 2);
    }
    std::string ext = std::filesystem::path{ filepath }.extension().string();

    Assimp::Importer importer;
    const aiScene*   scene = importer.ReadFileFromMemory(
        binary.data(), binary.size(), aiProcess_Triangulate | aiProcess_GenNormals, ext.c_str());
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        YG_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
        return nullptr;
    }
    return ProcessNode(scene->mRootNode, scene, meshName);
}

std::vector<uint8_t> MeshSerializer::Serialize(const Ref<Mesh>& mesh, const std::string& key) { return {}; }

} // namespace Yogi
