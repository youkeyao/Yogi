#include "runtime/resources/mesh_manager.h"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace Yogi {

void processMesh(aiMesh *mesh, const aiScene *scene)
{
    Mesh yg_mesh;
    yg_mesh.name = mesh->mName.C_Str();

    for (uint32_t i = 0; i < mesh->mNumVertices; i++) {
        Mesh::Vertex vertex;
        vertex.position = glm::vec3{ mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z };
        vertex.normal = glm::vec3{ mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z };
        vertex.texcoord = glm::vec3{ mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y, mesh->mTextureCoords[0][i].z };
        yg_mesh.vertices.push_back(vertex);
    }
    for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
        const aiFace &Face = mesh->mFaces[i];
        YG_ASSERT(Face.mNumIndices == 3, "Load mesh error!");
        yg_mesh.indices.push_back(Face.mIndices[0]);
        yg_mesh.indices.push_back(Face.mIndices[1]);
        yg_mesh.indices.push_back(Face.mIndices[2]);
    }

    MeshManager::add_mesh(yg_mesh);
}
void processNode(aiNode *node, const aiScene *scene)
{
    for (unsigned int i = 0; i < node->mNumMeshes; i++) {
        aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
        processMesh(mesh, scene);
    }
    for (unsigned int i = 0; i < node->mNumChildren; i++) {
        processNode(node->mChildren[i], scene);
    }
}

std::map<std::string, Ref<Mesh>> MeshManager::s_meshes{};

void MeshManager::init(const std::string &dir_path)
{
    if (std::filesystem::is_directory(dir_path)) {
        for (auto &directory_entry : std::filesystem::directory_iterator(dir_path)) {
            const auto &path = directory_entry.path();
            std::string filename = path.stem().string();
            std::string extension = path.extension().string();

            if (directory_entry.is_directory()) {
                init(path.string());
            } else if (extension == ".obj") {
                load_mesh(path.string());
            }
        }
    }
}
void MeshManager::clear()
{
    s_meshes.clear();
}

void MeshManager::load_mesh(const std::string &filepath)
{
    Assimp::Importer importer;
    const aiScene   *scene =
        importer.ReadFile(filepath, aiProcess_Triangulate | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);
    // check for errors
    if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {  // if is Not Zero
        YG_CORE_ERROR("ERROR::ASSIMP:: {0}", importer.GetErrorString());
        return;
    }
    processNode(scene->mRootNode, scene);
}

void MeshManager::add_mesh(Mesh mesh)
{
    s_meshes[mesh.name] = CreateRef<Mesh>(mesh);
}

const Ref<Mesh> &MeshManager::get_mesh(const std::string &name)
{
    if (s_meshes.find(name) != s_meshes.end()) {
        return s_meshes[name];
    }
    return s_meshes["quad"];
}

void MeshManager::each_mesh_name(std::function<void(std::string)> func)
{
    for (auto [mesh_name, mesh] : s_meshes) {
        func(mesh_name);
    }
}

}  // namespace Yogi