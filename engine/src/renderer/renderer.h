#pragma once

#include "renderer/shader.h"

namespace hazel
{

    enum RendererAPI
    {
        None = 0, OpenGL = 1
    };

    class Renderer
    {
    public:
        inline static RendererAPI get_api() { return s_renderer_api; }
    private:
        static RendererAPI s_renderer_api;
    };

}