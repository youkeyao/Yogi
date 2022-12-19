#pragma once
#include "renderer/renderer_api.h"

namespace hazel {

    class OpenGLRendererAPI : public RendererAPI
    {
    public:
        void init() override;
        void set_clear_color(const glm::vec4& color) override;
        void clear() override;

        void draw_indexed(const Ref<VertexArray>& vertex_array) override;
    };

}