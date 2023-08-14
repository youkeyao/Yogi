#include "runtime/systems/render_system.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    Ref<FrameBuffer> s_frame_buffer = nullptr;
    int s_width = 1280;
    int s_height = 720;

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_stats();

        RenderCommand::set_viewport(0, 0, s_width, s_height);
        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });

        scene->view_components<TransformComponent, CameraComponent>([&](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.zoom_level = std::max(camera.zoom_level, 0.25f);
            render_camera(camera, transform, scene);
        });
    }

    void RenderSystem::render_camera(const CameraComponent& camera, const TransformComponent& transform, Scene* scene)
    {
        Renderer::set_view_pos(glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)});
        if (camera.is_ortho)
            Renderer::set_projection_view_matrix(glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f) * glm::inverse((glm::mat4)transform.transform));
        else
            Renderer::set_projection_view_matrix(glm::perspective(camera.fov, camera.aspect_ratio, 0.1f, 100.0f) * glm::inverse((glm::mat4)transform.transform));

        // scene draw
        Ref<FrameBuffer> frame_buffer = nullptr;
        if (camera.render_target != nullptr)
            frame_buffer = FrameBuffer::create(s_width, s_height, {camera.render_target});
        else
            frame_buffer = s_frame_buffer;
        if (frame_buffer) frame_buffer->bind();
        RenderCommand::clear();
        scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
            Renderer::draw_mesh(mesh_renderer.mesh, mesh_renderer.material, transform.transform, entity);
        });
        Renderer::draw_skybox(transform.transform);
        Renderer::flush();
        if (frame_buffer) frame_buffer->unbind();
    }

    void RenderSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizeEvent>(RenderSystem::on_window_resized, scene);
        dispatcher.dispatch<WindowCloseEvent>(RenderSystem::on_window_close, scene);
    }

    bool RenderSystem::on_window_resized(WindowResizeEvent& e, Scene* scene)
    {
        s_width = e.get_width();
        s_height = e.get_height();
        s_frame_buffer->resize(s_width, s_height);
        scene->view_components<TransformComponent, CameraComponent>([e](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.aspect_ratio = (float)e.get_width() / e.get_height();
        });
        return false;
    }

    bool RenderSystem::on_window_close(WindowCloseEvent& e, Scene* scene)
    {
        if (s_frame_buffer) s_frame_buffer.reset();
        return false;
    }

    void RenderSystem::set_default_frame_texture(const Ref<RenderTexture>& frame_texture)
    {
        s_frame_buffer = FrameBuffer::create(s_width, s_height, {frame_texture});
    }

}