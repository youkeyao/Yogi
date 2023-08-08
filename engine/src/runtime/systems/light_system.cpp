#include "runtime/systems/light_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"

namespace Yogi {

    void LightSystem::on_update(Timestep ts, Scene* scene)
    {
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
    }

}