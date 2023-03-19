#pragma once

#include <glm/glm.hpp>
#include "runtime/renderer/texture.h"
#include "runtime/renderer/pipeline.h"

namespace Yogi {

    class Renderer
    {
    public:
        static void init();
        static void shutdown();
        static void on_window_resize(uint32_t width, uint32_t height);

        static void set_pipeline(Ref<Pipeline> pipeline);
        static void set_projection_view_matrix(glm::mat4 projection_view_matrix);
        static void flush();
        static void draw_mesh(const std::string& name, const glm::mat4& transform, const Ref<Texture2D>& texture, const glm::vec4& color);
    };

}