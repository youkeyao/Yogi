#include "base/systems/render_system.h"
#include "base/scene/components.h"
#include "base/renderer/renderer_2d.h"
#include "base/renderer/render_command.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        RenderCommand::clear();
        
        scene->view_components<TransformComponent, SpriteRendererComponent>([ts](TransformComponent& transform, SpriteRendererComponent& sprite){
            glm::mat4 transform_matrix = glm::translate(glm::mat4(1.0f), transform.translation) *
                glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.y), glm::vec3(0, 1, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.z), glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), transform.scale);
            Renderer2D::draw_quad(transform_matrix, sprite.texture, {sprite.texcoords_min, sprite.texcoords_max}, sprite.color);
        });

        Renderer2D::flush();
    }

}