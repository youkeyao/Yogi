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
            Renderer2D::draw_quad(transform.transform, sprite.texture, {sprite.tex_min, sprite.tex_max}, sprite.color);
        });

        Renderer2D::flush();
    }

}