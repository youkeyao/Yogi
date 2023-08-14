#include "runtime/systems/light_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Yogi {

    Ref<FrameBuffer> s_shadow_frame_buffer;

    void LightSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_lights();
        if (!s_shadow_frame_buffer) s_shadow_frame_buffer = FrameBuffer::create(2048, 2048, {Renderer::get_shadow_map()});
        scene->view_components<TransformComponent, DirectionalLightComponent>([&](Entity entity, TransformComponent& transform, DirectionalLightComponent& light){
            Renderer::set_directional_light(light.color, glm::vec3{((glm::mat4)transform.transform * glm::vec4(0, 0, -1, 0))});
            glm::mat4 light_space_matrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 10.0f) * glm::inverse((glm::mat4)transform.transform);
            Renderer::set_projection_view_matrix(light_space_matrix);
            Renderer::set_light_space_matrix(light_space_matrix);
            s_shadow_frame_buffer->bind();
            RenderCommand::set_viewport(0, 0, 2048, 2048);
            RenderCommand::clear();
            scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
                Renderer::draw_mesh(mesh_renderer.mesh, MaterialManager::get_material("shadow"), transform.transform, entity);
            });
            Renderer::flush();
            s_shadow_frame_buffer->unbind();
        });
        scene->view_components<TransformComponent, SpotLightComponent>([&](Entity entity, TransformComponent& transform, SpotLightComponent& light){
            Renderer::add_spot_light({light.color, glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.cutoff});
        });
        scene->view_components<TransformComponent, PointLightComponent>([&](Entity entity, TransformComponent& transform, PointLightComponent& light){
            Renderer::add_point_light({glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.attenuation_parm, light.color});
        });
    }

    void LightSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowCloseEvent>(LightSystem::on_window_close, scene);
    }

    bool LightSystem::on_window_close(WindowCloseEvent& e, Scene* scene)
    {
        s_shadow_frame_buffer.reset();
        return false;
    }

}