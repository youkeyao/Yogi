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
        static void add_mesh(std::string name, Mesh mesh);
    private:
        static std::map<std::string, Mesh> s_meshes;
    };

}