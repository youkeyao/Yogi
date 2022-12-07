#include "renderer/renderer.h"
#include "renderer/renderer_command.h"

namespace hazel {

    void Renderer::begin_scene()
    {

    }

    void Renderer::end_scene()
    {

    }

    void Renderer::submit(const std::shared_ptr<VertexArray>& vertex_array)
    {
        vertex_array->bind();
        RendererCommand::draw_indexed(vertex_array);
    }

}