#pragma once

#include "renderer/renderer_api.h"
#include "renderer/vertex_array.h"

namespace hazel {

    class RendererCommand
    {
    public:
        inline static void init()
        {
            ms_renderer_api->init();
        }

        inline static void set_viewport(uint32_t x, uint32_t y, uint32_t width, uint32_t height) {
            ms_renderer_api->set_viewport(x, y, width, height);
        }

        inline static void set_clear_color(const glm::vec4& color)
        {
            ms_renderer_api->set_clear_color(color);
        }
        
        inline static void clear()
        {
            ms_renderer_api->clear();
        }

        inline static void draw_indexed(const Ref<VertexArray>& vertex_array)
        {
            ms_renderer_api->draw_indexed(vertex_array);
        }

    private:
        static RendererAPI* ms_renderer_api;
    };

}