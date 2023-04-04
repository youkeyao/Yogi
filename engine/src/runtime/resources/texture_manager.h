#pragma once

#include "runtime/renderer/texture.h"

namespace Yogi {

    class TextureManager
    {
    public:
        static void init(const std::string& dir_path);
        static void clear();
        static void add_texture(const std::string& name, const std::string& filepath);
        static const Ref<Texture2D>& get_texture(const std::string& name) { return s_textures[name]; }
    private:
        static std::map<std::string, Ref<Texture2D>> s_textures;
    };

}