#include "runtime/resources/texture_manager.h"
#include <filesystem>

namespace Yogi {

    std::map<std::string, Ref<Texture2D>> TextureManager::s_textures{};

    std::string base_path = "";

    void TextureManager::init(const std::string& dir_path)
    {
        if (std::filesystem::is_directory(dir_path)) {
            for (auto& directory_entry : std::filesystem::directory_iterator(dir_path)) {
                const auto& path = directory_entry.path();
                std::string filename = path.stem().string();

                if (directory_entry.is_directory()) {
                    base_path += filename + "/";
                    init(path.string());
                }
                else {
                    add_texture(base_path + filename, path.string());
                }
            }
        }
    }

    void TextureManager::clear()
    {
        s_textures.clear();
    }

    void TextureManager::add_texture(const std::string& name, const std::string& filepath)
    {
        s_textures[name] = Texture2D::create(filepath);
    }

}