#pragma once

#include "runtime/renderer/texture.h"

namespace Yogi {

    class TextureManager
    {
    public:
        static void init(const std::string& dir_path);
        static void clear();
        static void add_texture(const Ref<Texture2D>& texture);
        static const Ref<Texture2D>& get_texture(const std::string& key);
        static void each_texture(std::function<void(const Ref<Texture2D>&)> func);
    private:
        static std::map<std::string, Ref<Texture2D>> s_textures;
    };

}