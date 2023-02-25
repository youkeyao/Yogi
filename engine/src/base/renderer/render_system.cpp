#include "base/renderer/render_system.h"
#include "base/scene/component_manager.h"
#include "base/renderer/renderer_2d.h"
#include "base/renderer/render_command.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        // RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        // RenderCommand::clear();
        
        // scene->view_components<TransformComponent, SpriteRendererComponent>([ts](TransformComponent& transform, SpriteRendererComponent& sprite){
        //     Renderer2D::draw_quad(transform.translation, transform.rotation.z, {transform.scale.x, transform.scale.y}, sprite.texture, sprite.color);
        // });

        // Renderer2D::flush();
    }

}