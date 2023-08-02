#pragma once

#include "runtime/renderer/pipeline.h"
#include "runtime/renderer/texture.h"

namespace Yogi {

    class TextureManager
    {
    public:
        static void init(const std::string& dir_path);
        static void clear();
        static void add_texture(const Ref<Texture2D>& texture);
        static void add_render_texture(const Ref<RenderTexture>& texture);

        static const Ref<Texture2D>& get_texture(const std::string& key);
        static const Ref<RenderTexture>& get_render_texture(const std::string& key);

        static void each_texture(std::function<void(const Ref<Texture2D>&)> func);
        static void each_render_texture(std::function<void(const Ref<RenderTexture>&)> func);
    private:
        static std::map<std::string, Ref<Texture2D>> s_textures;
        static std::map<std::string, Ref<RenderTexture>> s_render_textures;
    };

}