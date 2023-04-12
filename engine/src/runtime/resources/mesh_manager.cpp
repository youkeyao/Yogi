#include "runtime/resources/mesh_manager.h"

namespace Yogi {

    std::map<std::string, Ref<Mesh>> MeshManager::s_meshes{};

    void MeshManager::init()
    {
        add_mesh({"quad", {
            {{ -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f }},
            {{ 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f }},
            {{ 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f }},
            {{ -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f }}},
        { 0, 1, 2, 2, 3, 0 }});
    }

    void MeshManager::add_mesh(Mesh mesh)
    {
        s_meshes[mesh.name] = CreateRef<Mesh>(mesh);
    }

    void MeshManager::each_mesh_name(std::function<void(std::string)> func)
    {
        for (auto [mesh_name, mesh] : s_meshes) {
            func(mesh_name);
        }
    }

}