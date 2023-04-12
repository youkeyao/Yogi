#pragma once

#include <glm/glm.hpp>

namespace Yogi {

    struct Mesh
    {
        std::string name;
        std::vector<std::pair<glm::vec3, glm::vec2>> vertices;
        std::vector<uint32_t> indices;
    };

    class MeshManager
    {
    public:
        static void init();
        static void load_mesh(const std::string& filepath);
        static void add_mesh(Mesh mesh);
        static const Ref<Mesh>& get_mesh(const std::string& name) { return s_meshes[name]; }
        static void each_mesh_name(std::function<void(std::string)> func);
    private:
        static std::map<std::string, Ref<Mesh>> s_meshes;
    };

}