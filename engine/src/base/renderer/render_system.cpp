#include "base/renderer/render_system.h"
#include "base/renderer/renderer_2d.h"

namespace Yogi {

    void RenderSystem::on_update(TransformComponent transform, SpriteRendererComponent sprite)
    {
        Renderer2D::draw_quad(transform.translation, transform.rotation.z, {transform.scale.x, transform.scale.y}, sprite.texture, sprite.color);
    }

}