#include "runtime/systems/render_system.h"
#include "runtime/renderer/renderer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/frame_buffer.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

    int RenderSystem::s_width = 1280;
    int RenderSystem::s_height = 720;
    FrameBuffer* RenderSystem::s_frame_buffer = nullptr;

    RenderSystem::RenderSystem()
    {
        m_shadow_frame_buffer = FrameBuffer::create(m_shadow_map_size, m_shadow_map_size, { Renderer::get_shadow_map() });
    }
    RenderSystem::~RenderSystem()
    {
        m_shadow_frame_buffer.reset();
    }

    void RenderSystem::on_update(Timestep ts, Scene* scene)
    {
        Renderer::reset_stats();

        RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });

        // light
        set_light(scene);

        // render camera
        scene->view_components<TransformComponent, CameraComponent>([&](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.zoom_level = std::max(camera.zoom_level, 0.25f);
            render_camera(camera, transform, scene);
        });
    }

    void RenderSystem::set_light(Scene* scene)
    {
        Renderer::reset_lights();
        scene->view_components<TransformComponent, DirectionalLightComponent>([&](Entity entity, TransformComponent& transform, DirectionalLightComponent& light){
            Renderer::set_directional_light(light.color, glm::vec3{((glm::mat4)transform.transform * glm::vec4(0, 0, -1, 0))});
            glm::mat4 light_space_matrix = glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 10.0f) * glm::inverse((glm::mat4)transform.transform);
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
            Renderer::add_spot_light({light.color, glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.cutoff, (glm::mat3)transform.transform * glm::vec3(0, 0, -1), light.attenuation_parm});
        });
        scene->view_components<TransformComponent, PointLightComponent>([&](Entity entity, TransformComponent& transform, PointLightComponent& light){
            Renderer::add_point_light({glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)}, light.attenuation_parm, light.color});
        });
    }

    void RenderSystem::render_camera(const CameraComponent& camera, const TransformComponent& transform, Scene* scene)
    {
        Renderer::set_view_pos(glm::vec3{(glm::mat4)transform.transform * glm::vec4(0, 0, 0, 1)});
        if (camera.is_ortho)
            Renderer::set_projection_view_matrix(glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f) * glm::inverse((glm::mat4)transform.transform));
        else
            Renderer::set_projection_view_matrix(glm::perspective(camera.fov, camera.aspect_ratio, 0.1f, 500.0f) * glm::inverse((glm::mat4)transform.transform));

        // scene draw
        RenderCommand::set_viewport(0, 0, s_width, s_height);

        Ref<FrameBuffer> frame_buffer = nullptr;
        if (camera.render_target != nullptr) {
            frame_buffer = FrameBuffer::create(s_width, s_height, {camera.render_target});
            frame_buffer->bind();
        }
        else if (s_frame_buffer) {
            s_frame_buffer->bind();
        }

        RenderCommand::clear();
        scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
            TransformComponent tmp_transform = transform;
            glm::mat4 transform_mat = transform.transform;
            while (tmp_transform.parent) {
                tmp_transform = tmp_transform.parent.get_component<TransformComponent>();
                transform_mat = (glm::mat4)tmp_transform.transform * transform_mat;
            }
            Renderer::draw_mesh(mesh_renderer.mesh, mesh_renderer.material, transform_mat, entity);
        });
        Renderer::flush();

        if (frame_buffer) frame_buffer->unbind();
        else if (s_frame_buffer) s_frame_buffer->unbind();
    }

    void RenderSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(RenderSystem::on_window_resized, std::placeholders::_2), scene);
    }

    bool RenderSystem::on_window_resized(WindowResizeEvent& e, Scene* scene)
    {
        s_width = e.get_width();
        s_height = e.get_height();
        if (s_frame_buffer) s_frame_buffer->resize(s_width, s_height);
        scene->view_components<TransformComponent, CameraComponent>([e](Entity entity, TransformComponent& transform, CameraComponent& camera){
            camera.aspect_ratio = (float)e.get_width() / e.get_height();
        });
        return false;
    }

    void RenderSystem::set_default_frame_buffer(const Ref<FrameBuffer>& frame_buffer)
    {
        s_frame_buffer = frame_buffer.get();
    }

}