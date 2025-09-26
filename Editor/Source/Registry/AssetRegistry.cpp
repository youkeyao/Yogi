#include "Registry/AssetRegistry.h"

#include <Yogi.h>

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Yogi
{

void ProcessNode(aiNode* node, const aiScene* scene, const std::string& filePath, std::vector<std::string>& keys)
{
    for (int i = 0; i < node->mNumMeshes; ++i)
    {
        aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
        keys.push_back(filePath + "::" + mesh->mName.C_Str());
    }
    for (int i = 0; i < node->mNumChildren; ++i)
    {
        ProcessNode(node->mChildren[i], scene, filePath, keys);
    }
}
void RegisterMeshes(const std::filesystem::path& path, std::unordered_map<uint32_t, std::vector<std::string>>& keyMaps)
{
    if (path.extension().string() == ".obj")
    {
        Assimp::Importer importer;
        const aiScene*   scene = importer.ReadFile(
            path.string(), aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
        if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode)
        {
            YG_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
            return;
        }
        ProcessNode(scene->mRootNode, scene, path.string(), keyMaps[GetTypeHash<Mesh>()]);
    }
}
void RegisterMaterials(const std::filesystem::path&                            path,
                       std::unordered_map<uint32_t, std::vector<std::string>>& keyMaps)
{
    if (path.extension().string() == ".mat")
    {
        keyMaps[GetTypeHash<Material>()].push_back(path.string());
    }
}
void RegisterShaders(const std::filesystem::path& path, std::unordered_map<uint32_t, std::vector<std::string>>& keyMaps)
{
    if (path.extension().string() == ".vert" || path.extension().string() == ".frag")
    {
        keyMaps[GetTypeHash<ShaderDesc>()].push_back((path.parent_path() / path.stem()).string());
    }
}

// --------------------------------------------------------------------------------

std::vector<AssetRegistry::RegisterKeyFunc>            AssetRegistry::s_registerKeyFuncs;
std::unordered_map<uint32_t, std::vector<std::string>> AssetRegistry::s_keyMaps;

void AssetRegistry::Init()
{
    Register(&RegisterMeshes);
    Register(&RegisterMaterials);
    Register(&RegisterShaders);
}

void AssetRegistry::Scan(const std::string& rootDir)
{
    if (std::filesystem::is_directory(rootDir))
    {
        for (auto& entry : std::filesystem::recursive_directory_iterator(rootDir))
        {
            if (entry.is_regular_file())
            {
                std::filesystem::path relPath = std::filesystem::relative(entry.path(), rootDir);
                for (auto& func : s_registerKeyFuncs)
                {
                    func(relPath, s_keyMaps);
                }
            }
        }
    }
}

void AssetRegistry::Register(RegisterKeyFunc&& func) { s_registerKeyFuncs.push_back(std::move(func)); }
void AssetRegistry::Clear() { s_keyMaps.clear(); }

} // namespace Yogi
