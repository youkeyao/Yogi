#pragma once

#include <glm/glm.hpp>

namespace Yogi {

    struct Mesh
    {
        struct Vertex
        {
            glm::vec3 position;
            glm::vec3 normal;
            glm::vec2 texcoord;
        };
        
        std::string name;
        std::vector<Vertex> vertices;
        std::vector<uint32_t> indices;
    };

    class MeshManager
    {
    public:
        static void init(const std::string& dir_path);
        static void load_mesh(const std::string& filepath);
        static void add_mesh(Mesh mesh);
        static const Ref<Mesh>& get_mesh(const std::string& name);
        static void each_mesh_name(std::function<void(std::string)> func);
    private:
        static std::map<std::string, Ref<Mesh>> s_meshes;
    };

}