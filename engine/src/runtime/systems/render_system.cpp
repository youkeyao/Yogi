#include "runtime/systems/render_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/events/application_event.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    uint32_t s_width = 1280;
    uint32_t s_height = 720;

    Ref<FrameBuffer> get_frame_buffer(const Ref<FrameBuffer>& frame_buffer)
    {
        if (frame_buffer) {
            if (frame_buffer->get_width() != s_width || frame_buffer->get_height() != s_height) {
                frame_buffer->resize(s_width, s_height);
            }
            return frame_buffer;
        }
        else {
            return nullptr;
        }
    }

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_stats();

        std::vector<std::pair<Ref<Material>, Ref<FrameBuffer>>> render_passes = scene->get_render_passes();

        Ref<FrameBuffer> frame_buffer = get_frame_buffer(render_passes[0].second);
        if (frame_buffer) {
            frame_buffer->bind();
        }

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        RenderCommand::clear();
        scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
            Renderer::draw_mesh(mesh_renderer.mesh, mesh_renderer.material, transform.transform, entity);
        });
        Renderer::each_pipeline([&](const Ref<Pipeline>& pipeline){
            Renderer::flush_pipeline(pipeline);
        });

        if (frame_buffer) {
            frame_buffer->unbind();
        }

        for (int32_t i = 1; i < render_passes.size(); i ++) {
            auto& [material, framebuffer] = render_passes[i];
            Ref<FrameBuffer> frame_buffer = get_frame_buffer(framebuffer);
            if (frame_buffer) {
                frame_buffer->bind();
            }
            RenderCommand::clear();
            Renderer::draw_mesh(MeshManager::get_mesh("render_quad"), material, glm::mat4(1.0f), 0);
            Renderer::flush_pipeline(material->get_pipeline());
            if (frame_buffer) {
                frame_buffer->unbind();
            }
        }
    }

    bool on_render_system_window_resized(WindowResizeEvent& e, Scene* scene)
    {
        s_width = e.get_width();
        s_height = e.get_height();
        RenderCommand::set_viewport(0, 0, e.get_width(), e.get_height());
        return false;
    }

    void RenderSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizeEvent>(on_render_system_window_resized, scene);
    }

}