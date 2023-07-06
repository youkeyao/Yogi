#include "runtime/resources/material_manager.h"
#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/texture_manager.h"
#include "runtime/utility/md5.h"

namespace Yogi {

    std::map<std::string, Ref<Material>> MaterialManager::s_materials{};

    void MaterialManager::init(const std::string& dir_path)
    {
        if (std::filesystem::is_directory(dir_path)) {
            for (auto& directory_entry : std::filesystem::directory_iterator(dir_path)) {
                const auto& path = directory_entry.path();
                std::string filename = path.stem().string();
                std::string extension = path.extension().string();

                if (directory_entry.is_directory()) {
                    init(path.string());
                }
                else if (extension == ".mat") {
                    load_material(path.string());
                }
            }
        }
    }

    void MaterialManager::clear()
    {
        s_materials.clear();
    }

    void MaterialManager::add_material(const std::string& key, const Ref<Material>& material)
    {
        s_materials[key] = material;
    }

    void MaterialManager::remove_material(const Ref<Material>& material)
    {
        for (auto& [key, value] : s_materials) {
            if (value == material) {
                s_materials.erase(key);
                break;
            }
        }
    }

    const Ref<Material>& MaterialManager::get_material(const std::string& key)
    {
        if (s_materials.find(key) != s_materials.end()) {
            return s_materials[key];
        }
        return s_materials["undefined:277b765c80aebee40ae3e157ec7ab21c"];
    }

    void MaterialManager::each_material(std::function<void(const Ref<Material>&)> func)
    {
        for (auto& [key, material] : s_materials) {
            func(material);
        }
    }

    void MaterialManager::save_material(const std::string& path, const Ref<Material>& material)
    {
        std::ofstream out(path + "/" + material->get_name() + ".mat", std::ios::out);
        if (!out) {
            YG_CORE_ERROR("Could not open file '{0}'!", path + "/" + material->get_name() + ".mat");
            return;
        }
        std::string data = serialize_material(material);
        out << data;
        out.close();

        add_material(material->get_name() + ":" + MD5(data).toStr(), material);
    }

    Ref<Material> MaterialManager::load_material(const std::string& path)
    {
        std::filesystem::path fs_path = path;
        std::string name = fs_path.stem().string();
        std::ifstream in(path, std::ios::in);
        if (!in) {
            YG_CORE_WARN("Could not open file '{0}'!", path);
            return nullptr;
        }

        in.seekg(0, std::ios::end);
        uint32_t size = in.tellg();
        in.seekg(0, std::ios::beg);
        std::string file_data;
        file_data.resize(size);
        in.read(file_data.data(), size);
        in.close();

        // if exists, return
        std::string digest = MD5(file_data).toStr();
        if (s_materials.find(name + ":" + digest) != s_materials.end()) {
            return s_materials[name + ":" + digest];
        }

        // load
        std::stringstream ss(file_data);
        std::string pipeline_name;
        std::string data;
        ss >> pipeline_name;
        Ref<Pipeline> pipeline = PipelineManager::get_pipeline(pipeline_name);
        Ref<Material> material = CreateRef<Material>(name, pipeline);
        int32_t textures_size;
        ss >> textures_size;
        char c;
        for (int32_t i = 0; i < textures_size; i ++) {
            c = ss.get();
            c = ss.peek();
            if (c != ' ' && c != '\n') {
                std::string texture_name;
                ss >> texture_name;
                material->set_texture(i, TextureManager::get_texture(texture_name));
            }
        }
        ss >> data;

        memcpy(material->get_data(), data.data(), pipeline->get_vertex_layout().get_stride());
        add_material(name + ":" + digest, material);
        return material;
    }

    std::string MaterialManager::get_key(const Ref<Material>& material)
    {
        return material->get_name() + ":" + MD5(serialize_material(material)).toStr();
    }

    std::string MaterialManager::serialize_material(const Ref<Material>& material)
    {
        std::stringstream ss;
        Ref<Pipeline> pipeline = material->get_pipeline();
        uint32_t stride = pipeline->get_vertex_layout().get_stride();

        ss << pipeline->get_name() << std::endl;
        std::vector<std::pair<uint32_t, Ref<Texture2D>>> textures = material->get_textures();
        ss << textures.size();
        for (auto& [offset, texture] : textures) {
            if (texture) {
                ss << " " << texture->get_name() + ":" + texture->get_digest();
            }
            else {
                ss << " ";
            }
        }
        ss << std::endl;
        std::string data(material->get_data(), material->get_data() + stride);
        ss << data;

        return ss.str();
    }

}