#pragma once

#include "runtime/renderer/buffer.h"
#include <glm/glm.hpp>

namespace Yogi {

    class RenderCommand
    {
    public:
        static void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height);
        static void set_clear_color(const glm::vec4& color = { 0.0f, 0.0f, 0.0f, 1.0f });
        static void clear();
        static void draw_indexed(uint32_t count);
    };

}