#pragma once

#include "renderer/vertex_array.h"
#include <glm/glm.hpp>

namespace hazel {

    class RendererAPI
    {
    public:
        enum class API
        {
            None = 0, OpenGL = 1
        };
        virtual void init() = 0;
        virtual void set_clear_color(const glm::vec4& color) = 0;
        virtual void clear() = 0;

        virtual void draw_indexed(const Ref<VertexArray>& vertex_array) = 0;

        inline static API get_api() { return ms_api; }
    private:
        static API ms_api;
    };

}