#include "base/renderer/render_system.h"
#include "base/scene/component_manager.h"
#include "base/renderer/renderer_2d.h"
#include "base/renderer/render_command.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        RenderCommand::clear();
        
        scene->view_components({"TransformComponent", "SpriteRendererComponent"}, [ts](std::vector<void*> components){
            glm::vec3& translation = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "translation");
            glm::vec3& rotation = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "rotation");
            glm::vec3& scale = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "scale");
            glm::vec4& color = ComponentManager::field<glm::vec4>(components[1], "SpriteRendererComponent", "color");
            Renderer2D::draw_quad(translation, rotation.z, {scale.x, scale.y}, color);
        });

        Renderer2D::flush();
    }

}