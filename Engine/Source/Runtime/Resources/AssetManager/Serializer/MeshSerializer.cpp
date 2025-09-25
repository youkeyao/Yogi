#include "Resources/AssetManager/Serializer/MeshSerializer.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Yogi
{

Handle<Mesh> ProcessMesh(aiMesh* mesh, const aiScene* scene)
{
    std::vector<Vertex>   vertices;
    std::vector<uint32_t> indices;

    for (int i = 0; i < mesh->mNumVertices; ++i)
    {
        Vertex vertex;
        vertex.Position = Vector3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.Normal   = Vector3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.Texcoord = Vector2{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y };
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

    return Handle<Mesh>::Create(vertices, indices);
}
Handle<Mesh> ProcessNode(aiNode* node, const aiScene* scene, const std::string& meshName)
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

Handle<Mesh> MeshSerializer::Deserialize(const std::vector<uint8_t>& binary, const std::string& key)
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
        binary.data(), binary.size(), aiProcess_Triangulate | aiProcess_FlipUVs | aiProcess_GenNormals, ext.c_str());
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
    {
        YG_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
        return nullptr;
    }
    return ProcessNode(scene->mRootNode, scene, meshName);
}

std::vector<uint8_t> MeshSerializer::Serialize(const Ref<Mesh>& mesh, const std::string& key) { return {}; }

} // namespace Yogi
