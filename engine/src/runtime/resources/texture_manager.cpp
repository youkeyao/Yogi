#include "runtime/resources/texture_manager.h"
#include <filesystem>

namespace Yogi {

    std::map<std::string, Ref<Texture2D>> TextureManager::s_textures{};
    std::map<std::string, Ref<RenderTexture>> TextureManager::s_render_textures{};

    void TextureManager::init(const std::string& dir_path)
    {
        if (std::filesystem::is_directory(dir_path)) {
            for (auto& directory_entry : std::filesystem::directory_iterator(dir_path)) {
                const auto& path = directory_entry.path();
                std::string filename = path.stem().string();
                std::string extension = path.extension().string();

                if (directory_entry.is_directory()) {
                    init(path.string());
                }
                else if (extension == ".png") {
                    add_texture(Texture2D::create(filename, path.string()));
                }
            }
        }
    }

    void TextureManager::clear()
    {
        s_textures.clear();
        s_render_textures.clear();
    }

    void TextureManager::add_texture(const Ref<Texture2D>& texture)
    {
        s_textures[texture->get_name() + ":" + texture->get_digest()] = texture;
    }

    void TextureManager::add_render_texture(const Ref<Pipeline>& pipeline, uint32_t index)
    {
        auto& elements = pipeline->get_output_layout().get_elements();
        if (index < elements.size()) {
            auto& element = elements[index];
            if (element.type == ShaderDataType::Int) {
                s_render_textures[pipeline->get_name() + "|" + element.name] = RenderTexture::create(pipeline->get_name() + "|" + element.name, 1, 1, TextureFormat::RED_INTEGER);
            }
            else if (element.type == ShaderDataType::Float4) {
                s_render_textures[pipeline->get_name() + "|" + element.name] = RenderTexture::create(pipeline->get_name() + "|" + element.name, 1, 1, TextureFormat::ATTACHMENT);
            }
        }
    }

    const Ref<Texture2D>& TextureManager::get_texture(const std::string& key)
    {
        if (s_textures.find(key) != s_textures.end()) {
            return s_textures[key];
        }
        return s_textures["checkerboard:9dc354f091dee664ebe9efdc88a98da8"];
    }

    void TextureManager::each_texture(std::function<void(const Ref<Texture2D>&)> func)
    {
        for (auto [texture_name, texture] : s_textures) {
            func(texture);
        }
    }

    void TextureManager::each_render_texture(std::function<void(const Ref<RenderTexture>&)> func)
    {
        for (auto [texture_name, texture] : s_render_textures) {
            func(texture);
        }
    }

}