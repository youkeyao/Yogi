#include "runtime/renderer/mesh_manager.h"

namespace Yogi {

    std::map<std::string, Mesh> MeshManager::s_meshes{};

    void MeshManager::init()
    {
        add_mesh("quad", {{
            {{ -0.5f, -0.5f, 0.0f }, { 0.0f, 0.0f }},
            {{ 0.5f, -0.5f, 0.0f }, { 1.0f, 0.0f }},
            {{ 0.5f, 0.5f, 0.0f }, { 1.0f, 1.0f }},
            {{ -0.5f, 0.5f, 0.0f }, { 0.0f, 1.0f }}},
        { 0, 1, 2, 2, 3, 0 }});
    }

    void MeshManager::add_mesh(const std::string& name, Mesh mesh)
    {
        s_meshes[name] = mesh;
    }

}