#include "runtime/renderer/render_system.h"

#include "runtime/renderer/frame_buffer.h"
#include "runtime/renderer/render_command.h"
#include "runtime/renderer/renderer.h"
#include "runtime/resources/material_manager.h"

namespace Yogi {

int   RenderSystem::s_width = 1280;
int   RenderSystem::s_height = 720;
float near = 0.5f;
float far = 50.0f;

FrameBuffer           *RenderSystem::s_frame_buffer = nullptr;
std::vector<glm::mat4> pointShadowTransforms = {
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f)),
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f)),
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec3(0.0f, -1.0f, 0.0f)),
    glm::perspective(glm::radians(90.0f), 1.0f, near, far) *
        glm::lookAt(glm::vec3(0.0f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec3(0.0f, -1.0f, 0.0f))
};

RenderSystem::RenderSystem() {}
RenderSystem::~RenderSystem()
{
    for (auto &frame_buffer : m_shadow_frame_buffer_pool) {
        auto shadow_map = frame_buffer->get_color_attachment(0);
        shadow_map.reset();
        frame_buffer.reset();
    }
}

void RenderSystem::on_update(Timestep ts, Scene &scene)
{
    Renderer::reset_stats();

    // light
    set_light(scene);

    // render camera
    scene.view_components<TransformComponent, CameraComponent>(
        [&](Entity entity, TransformComponent &transform, CameraComponent &camera) {
            camera.zoom_level = std::max(camera.zoom_level, 0.25f);
            render_camera(camera, transform, scene);
        });
}

void RenderSystem::set_light(Scene &scene)
{
    RenderCommand::set_clear_color({ 1.0f, 0.0f, 0.0f, 1.0f });

    m_shadow_frame_buffer_pool_index = 0;
    m_directional_lights.clear();
    m_spot_lights.clear();
    m_point_lights.clear();
    scene.view_components<TransformComponent, DirectionalLightComponent>(
        [&](Entity entity, TransformComponent &transform, DirectionalLightComponent &light) {
            const glm::mat4 world_transform = transform.get_world_transform();
            const glm::mat4 light_space_matrix =
                glm::ortho(-10.0f, 10.0f, -10.0f, 10.0f, -1.0f, 10.0f) * glm::inverse(world_transform);
            const uint32_t shadow_pool_index = allocate_shadow_frame_buffer();
            m_directional_lights.emplace_back(
                std::make_pair(light, ShadowData{ light_space_matrix, world_transform, shadow_pool_index }));
            m_shadow_frame_buffer_pool[shadow_pool_index]->bind();
            Renderer::set_projection_view_matrix(light_space_matrix);
            RenderCommand::set_viewport(0, 0, m_shadow_map_size, m_shadow_map_size);
            RenderCommand::clear();
            scene.view_components<TransformComponent, MeshRendererComponent>(
                [&](Entity mesh_entity, TransformComponent &mesh_transform, MeshRendererComponent &mesh_renderer) {
                    if (mesh_renderer.cast_shadow) {
                        Renderer::draw_mesh(
                            mesh_renderer.mesh, MaterialManager::get_material("shadow"), mesh_transform.get_world_transform(),
                            mesh_entity);
                    }
                });
            Renderer::flush();
            m_shadow_frame_buffer_pool[shadow_pool_index]->unbind();
        });
    scene.view_components<TransformComponent, SpotLightComponent>([&](Entity entity, TransformComponent &transform,
                                                                      SpotLightComponent &light) {
        const glm::mat4 world_transform = transform.get_world_transform();
        const glm::mat4 light_space_matrix =
            glm::perspective(glm::radians(light.outer_angle * 2), 1.0f, near, far) * glm::inverse(world_transform);
        const uint32_t shadow_pool_index = allocate_shadow_frame_buffer();
        m_spot_lights.emplace_back(std::make_pair(light, ShadowData{ light_space_matrix, world_transform, shadow_pool_index }));
        m_shadow_frame_buffer_pool[shadow_pool_index]->bind();
        Renderer::set_projection_view_matrix(light_space_matrix);
        RenderCommand::set_viewport(0, 0, m_shadow_map_size, m_shadow_map_size);
        RenderCommand::clear();
        scene.view_components<TransformComponent, MeshRendererComponent>(
            [&](Entity mesh_entity, TransformComponent &mesh_transform, MeshRendererComponent &mesh_renderer) {
                if (mesh_renderer.cast_shadow) {
                    Renderer::draw_mesh(
                        mesh_renderer.mesh, MaterialManager::get_material("shadow"), mesh_transform.get_world_transform(),
                        mesh_entity);
                }
            });
        Renderer::flush();
        m_shadow_frame_buffer_pool[shadow_pool_index]->unbind();
    });
    scene.view_components<TransformComponent, PointLightComponent>(
        [&](Entity entity, TransformComponent &transform, PointLightComponent &light) {
            std::array<ShadowData, 6> &data =
                m_point_lights.emplace_back(std::make_pair(light, std::array<ShadowData, 6>{})).second;
            for (int32_t i = 0; i < 6; i++) {
                ShadowData &shadow_data = data[i];
                shadow_data.light_transform = transform.get_world_transform();
                glm::vec3 light_pos = glm::vec3{ (glm::mat4)shadow_data.light_transform * glm::vec4(0, 0, 0, 1) };
                shadow_data.light_space_matrix = pointShadowTransforms[i] * glm::translate(glm::mat4(1.0f), -light_pos);
                shadow_data.shadow_pool_index = allocate_shadow_frame_buffer();
                m_shadow_frame_buffer_pool[shadow_data.shadow_pool_index]->bind();
                Renderer::set_projection_view_matrix(shadow_data.light_space_matrix);
                RenderCommand::set_viewport(0, 0, m_shadow_map_size, m_shadow_map_size);
                RenderCommand::clear();
                scene.view_components<TransformComponent, MeshRendererComponent>(
                    [&](Entity mesh_entity, TransformComponent &mesh_transform, MeshRendererComponent &mesh_renderer) {
                        if (mesh_renderer.cast_shadow) {
                            Renderer::draw_mesh(
                                mesh_renderer.mesh, MaterialManager::get_material("shadow"),
                                mesh_transform.get_world_transform(), mesh_entity);
                        }
                    });
                Renderer::flush();
                m_shadow_frame_buffer_pool[shadow_data.shadow_pool_index]->unbind();
            }
        });
}

void RenderSystem::render_camera(const CameraComponent &camera, const TransformComponent &transform, Scene &scene)
{
    Renderer::reset_lights();

    Renderer::set_view_pos(glm::vec3{ (glm::mat4)transform.get_world_transform() * glm::vec4(0, 0, 0, 1) });
    if (camera.is_ortho)
        Renderer::set_projection_view_matrix(
            glm::ortho(
                -camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level,
                camera.zoom_level, -1.0f, 1.0f) *
            glm::inverse((glm::mat4)transform.get_world_transform()));
    else
        Renderer::set_projection_view_matrix(
            glm::perspective(camera.fov, camera.aspect_ratio, 0.1f, 500.0f) *
            glm::inverse((glm::mat4)transform.get_world_transform()));

    // scene draw
    RenderCommand::set_viewport(0, 0, s_width, s_height);

    if (s_frame_buffer) {
        s_frame_buffer->bind();
    }

    RenderCommand::set_clear_color({ 0.1f, 0.1f, 0.1f, 1.0f });
    RenderCommand::clear();
    {
        std::vector<std::pair<DirectionalLightComponent, ShadowData>>::iterator directional_light_iterator =
            m_directional_lights.begin();
        std::vector<std::pair<SpotLightComponent, ShadowData>>::iterator spot_light_iterator = m_spot_lights.begin();
        std::vector<std::pair<PointLightComponent, std::array<ShadowData, 6>>>::iterator point_light_iterator =
            m_point_lights.begin();
        while (directional_light_iterator != m_directional_lights.end() || spot_light_iterator != m_spot_lights.end() ||
               point_light_iterator != m_point_lights.end()) {
            Renderer::set_current_texture_slot(0);
            if (directional_light_iterator != m_directional_lights.end()) {
                DirectionalLightComponent directional_light = directional_light_iterator->first;
                ShadowData                shadow_data = directional_light_iterator->second;
                Renderer::set_directional_light(
                    directional_light.color, glm::vec3{ (glm::mat4)shadow_data.light_transform * glm::vec4(0, 0, -1, 0) },
                    shadow_data.light_space_matrix);
                Renderer::add_texture(m_shadow_frame_buffer_pool[shadow_data.shadow_pool_index]->get_color_attachment(0));
                directional_light_iterator++;
            }
            while (spot_light_iterator != m_spot_lights.end()) {
                SpotLightComponent spot_light = spot_light_iterator->first;
                ShadowData         shadow_data = spot_light_iterator->second;
                if (!Renderer::add_spot_light(
                        { spot_light.color, glm::vec3{ (glm::mat4)shadow_data.light_transform * glm::vec4(0, 0, 0, 1) },
                          glm::cos(glm::radians(spot_light.inner_angle)),
                          glm::vec3{ (glm::mat4)shadow_data.light_transform * glm::vec4(0, 0, -1, 0) },
                          glm::cos(glm::radians(spot_light.outer_angle)), shadow_data.light_space_matrix }))
                    break;
                Renderer::add_texture(m_shadow_frame_buffer_pool[shadow_data.shadow_pool_index]->get_color_attachment(0));
                spot_light_iterator++;
            }
            while (point_light_iterator != m_point_lights.end()) {
                PointLightComponent       point_light = point_light_iterator->first;
                std::array<ShadowData, 6> shadow_data = point_light_iterator->second;
                if (!Renderer::add_point_light({ glm::vec3{ (glm::mat4)shadow_data[0].light_transform * glm::vec4(0, 0, 0, 1) },
                                                 point_light.attenuation_parm, point_light.color,
                                                 shadow_data[0].light_space_matrix }))
                    break;
                for (int32_t i = 0; i < 6; i++) {
                    Renderer::add_texture(
                        m_shadow_frame_buffer_pool[shadow_data[i].shadow_pool_index]->get_color_attachment(0));
                }
                point_light_iterator++;
            }
            Renderer::set_texture_init_slot(Renderer::get_current_texture_slot());
            scene.view_components<TransformComponent, MeshRendererComponent>(
                [&](Entity mesh_entity, TransformComponent &mesh_transform, MeshRendererComponent &mesh_renderer) {
                    Renderer::draw_mesh(
                        mesh_renderer.mesh, mesh_renderer.material, mesh_transform.get_world_transform(), mesh_entity);
                });
            Renderer::flush();
        }
    }

    // skybox
    scene.view_components<SkyboxComponent>([&](Entity skybox_entity, SkyboxComponent &skybox) {
        Renderer::draw_mesh(MeshManager::get_mesh("skybox"), skybox.material, glm::mat4(1.0f), skybox_entity);
    });
    Renderer::flush();

    if (s_frame_buffer)
        s_frame_buffer->unbind();
}

void RenderSystem::on_event(Event &e, Scene &scene)
{
    EventDispatcher dispatcher(e);
    dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(RenderSystem::on_window_resized, std::placeholders::_2), scene);
}

bool RenderSystem::on_window_resized(WindowResizeEvent &e, Scene &scene)
{
    s_width = e.get_width();
    s_height = e.get_height();
    if (s_frame_buffer)
        s_frame_buffer->resize(s_width, s_height);
    scene.view_components<TransformComponent, CameraComponent>(
        [e](Entity entity, TransformComponent &transform, CameraComponent &camera) {
            camera.aspect_ratio = (float)e.get_width() / e.get_height();
        });
    return false;
}

void RenderSystem::set_default_frame_buffer(const Ref<FrameBuffer> &frame_buffer)
{
    s_frame_buffer = frame_buffer.get();
}

uint32_t RenderSystem::allocate_shadow_frame_buffer()
{
    if (m_shadow_frame_buffer_pool_index < m_shadow_frame_buffer_pool.size()) {
        return m_shadow_frame_buffer_pool_index++;
    }
    Ref<RenderTexture> shadow_map = RenderTexture::create("", m_shadow_map_size, m_shadow_map_size, TextureFormat::ATTACHMENT);
    m_shadow_frame_buffer_pool.emplace_back(FrameBuffer::create(m_shadow_map_size, m_shadow_map_size, { shadow_map }));
    return m_shadow_frame_buffer_pool_index++;
}

}  // namespace Yogi