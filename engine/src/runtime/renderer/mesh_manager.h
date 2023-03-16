#pragma once

#include <glm/glm.hpp>

namespace Yogi {

    struct Mesh
    {
        std::vector<std::pair<glm::vec3, glm::vec2>> vertices;
        std::vector<uint32_t> indices;
    };

    class MeshManager
    {
    public:
        static void init();
        static void add_mesh(const std::string& name, Mesh mesh);
        static const Mesh& get_mesh(const std::string& name) { return s_meshes[name]; }
    private:
        static std::map<std::string, Mesh> s_meshes;
    };

}