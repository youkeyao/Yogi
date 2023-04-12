#include "runtime/systems/render_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/events/application_event.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
        RenderCommand::clear();

        scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
            Renderer::draw_mesh(mesh_renderer.mesh, mesh_renderer.material, transform.transform);
        });

        Renderer::flush();
    }

}