#include "editor_layer.h"
#include "editor_camera_controller_system.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"
#include "reflect/scene_manager.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yogi {

    static uint32_t s_max_viewport_size = 4096;

    EditorLayer::EditorLayer() : Layer("EditorLayer") {}

    void EditorLayer::on_attach()
    {
        YG_PROFILE_FUNCTION();

        ComponentManager::init();
        SystemManager::init();

        m_frame_texture = Texture2D::create(s_max_viewport_size, s_max_viewport_size);
        m_frame_buffer = FrameBuffer::create(s_max_viewport_size, s_max_viewport_size, { m_frame_texture });

        m_scene = SceneManager::load_scene("scene.yg");
        m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
    }

    void EditorLayer::on_detach()
    {
        YG_PROFILE_FUNCTION();
    }

    void EditorLayer::on_update(Timestep ts)
    {
        YG_PROFILE_FUNCTION();
        
        imgui_update();
        m_hierarchy_panel->on_imgui_render();

        Renderer2D::reset_stats();
        m_frame_buffer->bind();

        if (m_viewport_focused) {
            EditorCameraControllerSystem::on_update(ts, m_scene.get());
        }
        m_scene->on_update(ts);

        m_frame_buffer->unbind();
    }

    void EditorLayer::imgui_update()
    {
        YG_PROFILE_FUNCTION();

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("Exit")) {
                    Application::get().close();
                }
                if (ImGui::MenuItem("open")) {
                    m_scene = SceneManager::load_scene("scene.yg");
                    m_scene->register_system<CameraSystem>();
                    m_scene->register_system<RenderSystem>();
                    m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
                }
                if (ImGui::MenuItem("save")) {
                    SceneManager::save_scene(m_scene, "scene.yg");
                }
                
                ImGui::EndMenu();
            }

            ImGui::EndMenuBar();
        }

        Renderer2D::Statistics stats = Renderer2D::get_stats();
        ImGui::Begin("Stats");
        ImGui::Text("Renderer2D Stats:");
        ImGui::Text("Draw Calls: %d", stats.draw_calls);
        ImGui::Text("Quads: %d", stats.quad_count);
        ImGui::Text("Vertices: %d", stats.get_total_vertex_count());
        ImGui::Text("Indices: %d", stats.get_total_index_count());
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
        m_viewport_focused = ImGui::IsWindowFocused();
        ImVec2 viewport_region_min = ImGui::GetWindowContentRegionMin();
        ImVec2 viewport_region_max = ImGui::GetWindowContentRegionMax();
        ImVec2 viewport_offset = ImGui::GetWindowPos();
        m_viewport_bounds[0] = { viewport_region_min.x + viewport_offset.x, viewport_region_min.y + viewport_offset.y };
        m_viewport_bounds[1] = { viewport_region_max.x + viewport_offset.x, viewport_region_max.y + viewport_offset.y };
        glm::vec2 new_viewport_size = { viewport_region_max.x - viewport_region_min.x, viewport_region_max.y - viewport_region_min.y };
        if ((uint32_t)new_viewport_size.x < 0 || (uint32_t)new_viewport_size.y < 0 ||
            (uint32_t)new_viewport_size.x > s_max_viewport_size || (uint32_t)new_viewport_size.y > s_max_viewport_size
        ) {
            YG_CORE_WARN("Invalid viewport size!");
        }
        else if (new_viewport_size.x != m_viewport_size.x || new_viewport_size.y != m_viewport_size.y) {
            m_viewport_size = new_viewport_size;
            WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y);
            m_scene->on_event(e);
            EditorCameraControllerSystem::on_event(e, m_scene.get());
            RenderCommand::set_viewport(0.0f, 0.0f, m_viewport_size.x, m_viewport_size.y);
        }
        ImGui::Image(
            (void*)(uint64_t)m_frame_texture->get_renderer_id(),
            ImVec2(m_viewport_size.x, m_viewport_size.y),
            ImVec2( 0, m_viewport_size.y / m_frame_texture->get_height() ),
            ImVec2( m_viewport_size.x / m_frame_texture->get_width(), 0 )
        );
        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorLayer::on_event(Event& e)
    {
        YG_PROFILE_FUNCTION();
        
        if (e.get_event_type() != WindowResizeEvent::get_static_type()) {
            if (m_viewport_focused)
                EditorCameraControllerSystem::on_event(e, m_scene.get());
            m_scene->on_event(e);
        }
    }

}