#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/renderer_2d.h"

namespace Yogi {

    Renderer::SceneData* Renderer::m_scene_data = new Renderer::SceneData();

    void Renderer::init()
    {
        YG_PROFILE_FUNCTION();
        
        RenderCommand::init();
        Renderer2D::init();
    }

    void Renderer::shutdown()
    {
        Renderer2D::shutdown();
        delete m_scene_data;
    }

    void Renderer::on_window_resize(uint32_t width, uint32_t height)
    {
        RenderCommand::set_viewport(0, 0, width, height);
    }

    void Renderer::submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertex_array, const glm::mat4& transform)
    {
        shader->bind();

        vertex_array->bind();
        RenderCommand::draw_indexed(vertex_array);
    }

}