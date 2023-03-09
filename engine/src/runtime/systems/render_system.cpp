#include "runtime/systems/render_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer_2d.h"
#include "runtime/renderer/render_command.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        RenderCommand::clear();
        
        scene->view_components<TransformComponent, SpriteRendererComponent>([ts](Entity entity, TransformComponent& transform, SpriteRendererComponent& sprite){
            Renderer2D::draw_quad(transform.transform, sprite.texture, {sprite.tex_min, sprite.tex_max}, sprite.color, entity);
        });

        Renderer2D::flush();
    }

}