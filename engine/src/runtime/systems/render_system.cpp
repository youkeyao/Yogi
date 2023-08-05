#include "runtime/systems/render_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/events/application_event.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_stats();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });

        scene->view_components<TransformComponent, CameraComponent>([&](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.zoom_level = std::max(camera.zoom_level, 0.25f);
            Renderer::set_view_pos(glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)});
            if (camera.is_ortho)
                Renderer::set_projection_view_matrix(glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f) * glm::inverse((glm::mat4)transform.transform));
            else
                Renderer::set_projection_view_matrix(glm::perspective(camera.fov, camera.aspect_ratio, camera.zoom_level, 100.0f) * glm::inverse((glm::mat4)transform.transform));

            auto frame_buffer = scene->get_frame_buffer();
            if (frame_buffer) frame_buffer->bind();
            RenderCommand::clear();

            Renderer::reset_lights();
            scene->view_components<TransformComponent, DirectionalLightComponent>([&](Entity entity, TransformComponent& transform, DirectionalLightComponent& light){
                Renderer::set_directional_light(light.color, glm::vec3{((glm::mat4)transform.transform * glm::vec4(0, 0, 1, 0))});
            });
            scene->view_components<TransformComponent, SpotLightComponent>([&](Entity entity, TransformComponent& transform, SpotLightComponent& light){
                Renderer::add_spot_light({light.color, glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.cutoff});
            });
            scene->view_components<TransformComponent, PointLightComponent>([&](Entity entity, TransformComponent& transform, PointLightComponent& light){
                Renderer::add_point_light({glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.attenuation_parm, light.color});
            });
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