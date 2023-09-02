#include "runtime/systems/light_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace Yogi {

    LightSystem::LightSystem()
    {
        m_shadow_frame_buffer = FrameBuffer::create(m_shadow_map_size, m_shadow_map_size, { Renderer::get_shadow_map() });
    }
    LightSystem::~LightSystem()
    {
        m_shadow_frame_buffer.reset();
    }

    void LightSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_lights();
        scene->view_components<TransformComponent, DirectionalLightComponent>([&](Entity entity, TransformComponent& transform, DirectionalLightComponent& light){
            Renderer::set_directional_light(light.color, glm::vec3{((glm::mat4)transform.transform * glm::vec4(0, 0, -1, 0))});
            glm::mat4 light_space_matrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, 0.1f, 10.0f) * glm::inverse((glm::mat4)transform.transform);
            Renderer::set_light_space_matrix(light_space_matrix);
            m_shadow_frame_buffer->bind();
            RenderCommand::set_viewport(0, 0, m_shadow_map_size, m_shadow_map_size);
            RenderCommand::clear();
            scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
                if (mesh_renderer.cast_shadow) {
                    Renderer::draw_mesh(mesh_renderer.mesh, MaterialManager::get_material("shadow"), transform.transform, entity);
                }
            });
            Renderer::flush();
            m_shadow_frame_buffer->unbind();
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
    }

}