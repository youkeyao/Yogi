#include "runtime/resources/material_manager.h"

#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/texture_manager.h"

namespace Yogi {

std::map<std::string, Ref<Material>> MaterialManager::s_materials{};

void MaterialManager::init(const std::string &dir_path)
{
    if (std::filesystem::is_directory(dir_path)) {
        for (auto &directory_entry : std::filesystem::directory_iterator(dir_path)) {
            const auto &path = directory_entry.path();
            std::string filename = path.stem().string();
            std::string extension = path.extension().string();

            if (directory_entry.is_directory()) {
                init(path.string());
            } else if (extension == ".mat") {
                load_material(path.string());
            }
        }
    }
}

void MaterialManager::clear()
{
    s_materials.clear();
}

void MaterialManager::add_material(const std::string &key, const Ref<Material> &material)
{
    s_materials[key] = material;
}

void MaterialManager::remove_material(const Ref<Material> &material)
{
    for (auto &[key, value] : s_materials) {
        if (value == material) {
            s_materials.erase(key);
            break;
        }
    }
}

const Ref<Material> &MaterialManager::get_material(const std::string &key)
{
    if (s_materials.find(key) != s_materials.end()) {
        return s_materials[key];
    }
    return s_materials["undefined"];
}

void MaterialManager::each_material(std::function<void(const Ref<Material> &)> func)
{
    for (auto &[key, material] : s_materials) {
        func(material);
    }
}

void MaterialManager::save_material(const std::string &path, const Ref<Material> &material)
{
    std::ofstream out(path + "/" + material->get_name() + ".mat", std::ios::out);
    if (!out) {
        YG_CORE_ERROR("Could not open file '{0}'!", path + "/" + material->get_name() + ".mat");
        return;
    }
    std::string data = serialize_material(material);
    out << data;
    out.close();
}

Ref<Material> MaterialManager::load_material(const std::string &path)
{
    std::filesystem::path fs_path = path;
    std::string           name = fs_path.stem().string();
    std::ifstream         in(path, std::ios::in);
    if (!in) {
        YG_CORE_WARN("Could not open file '{0}'!", path);
        return nullptr;
    }

    // load
    std::string pipeline_name;
    std::string data;
    in >> pipeline_name;
    Ref<Pipeline> pipeline = PipelineManager::get_pipeline(pipeline_name);
    Ref<Material> material = Material::create(name, pipeline);
    int32_t       textures_size;
    in >> textures_size;
    char c;
    for (int32_t i = 0; i < textures_size; i++) {
        c = in.get();
        c = in.peek();
        if (c != ' ' && c != '\n') {
            std::string texture_name;
            in >> texture_name;
            material->set_texture(i, TextureManager::get_texture(texture_name));
        }
    }
    in >> data;
    in.close();

    memcpy(material->get_data(), data.data(), pipeline->get_vertex_layout().get_stride());
    std::string test(material->get_data(), material->get_data() + pipeline->get_vertex_layout().get_stride());
    add_material(name, material);
    return material;
}

std::string MaterialManager::serialize_material(const Ref<Material> &material)
{
    std::stringstream ss;
    Ref<Pipeline>     pipeline = material->get_pipeline();
    uint32_t          stride = pipeline->get_vertex_layout().get_stride();

    ss << pipeline->get_name() << std::endl;
    std::vector<std::pair<uint32_t, Ref<Texture>>> textures = material->get_textures();
    ss << textures.size();
    for (auto &[offset, texture] : textures) {
        Texture2D *texture2d_ptr = dynamic_cast<Texture2D *>(texture.get());
        if (texture2d_ptr) {
            ss << " " << texture2d_ptr->get_name() + ":" + texture2d_ptr->get_digest();
        } else {
            ss << " ";
        }
    }
    ss << std::endl;
    std::string data(material->get_data(), material->get_data() + stride);
    ss << data;

    return ss.str();
}

}  // namespace Yogi