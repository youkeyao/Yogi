#include "runtime/systems/render_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/events/application_event.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    static glm::mat4 last_camera_transform = glm::mat4(1.0f);
    static glm::mat4 last_camera_transform_inverse = glm::mat4(1.0f);
    static bool last_camera_ortho = false;
    static float last_camera_fov = 1.0f;
    static float last_camera_aspect_ratio = 1.0f;
    static float last_camera_zoom_level = 1.0f;
    static glm::mat4 last_camera_projection = glm::mat4(1.0f);
    static glm::mat4 last_camera_projection_view = glm::mat4(1.0f);

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_stats();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });

        scene->view_components<TransformComponent, CameraComponent>([&](Entity entity, TransformComponent& transform, CameraComponent& camera){
            if ((glm::mat4)transform.transform != last_camera_transform) {
                last_camera_transform = transform.transform;
                last_camera_transform_inverse = glm::inverse(last_camera_transform);
                last_camera_projection_view = last_camera_projection * last_camera_transform_inverse;
            }
            if (camera.is_ortho != last_camera_ortho || camera.fov != last_camera_fov ||
                camera.aspect_ratio != last_camera_aspect_ratio || camera.zoom_level != last_camera_zoom_level
            ) {
                camera.zoom_level = std::max(camera.zoom_level, 0.25f);
                last_camera_ortho = camera.is_ortho;
                last_camera_fov = camera.fov;
                last_camera_aspect_ratio = camera.aspect_ratio;
                last_camera_zoom_level = camera.zoom_level;
                if (camera.is_ortho)
                    last_camera_projection = glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f);
                else
                    last_camera_projection = glm::perspective(camera.fov, camera.aspect_ratio, camera.zoom_level, 100.0f);
                last_camera_projection_view = last_camera_projection * last_camera_transform_inverse;
            }
            Renderer::set_projection_view_matrix(last_camera_projection_view);

            auto frame_buffer = scene->get_frame_buffer();
            if (frame_buffer) frame_buffer->bind();
            RenderCommand::clear();
            scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
                Renderer::draw_mesh(mesh_renderer.mesh, mesh_renderer.material, transform.transform, entity);
            });
            Renderer::flush();
            if (frame_buffer) frame_buffer->unbind();
        });
    }

    bool on_render_system_window_resized(WindowResizeEvent& e, Scene* scene)
    {
        RenderCommand::set_viewport(0, 0, e.get_width(), e.get_height());
        scene->view_components<TransformComponent, CameraComponent>([e](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.aspect_ratio = (float)e.get_width() / e.get_height();
        });
        return false;
    }

    void RenderSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizeEvent>(on_render_system_window_resized, scene);
    }

}