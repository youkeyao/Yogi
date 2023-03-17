#include "editor_layer.h"
#include "imgui_setting.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"
#include "reflect/scene_manager.h"
#include <glm/gtc/type_ptr.hpp>
#include <portable-file-dialogs.h>

namespace Yogi {

    static uint32_t s_max_viewport_size = 2048;

    EditorLayer::EditorLayer() : Layer("EditorLayer") {}

    void EditorLayer::on_attach()
    {
        YG_PROFILE_FUNCTION();
        ImguiSetting::init();

        ComponentManager::init();
        SystemManager::init();

        m_frame_texture = Texture2D::create(s_max_viewport_size, s_max_viewport_size);
        m_entity_id_texture = Texture2D::create(s_max_viewport_size, s_max_viewport_size, TextureFormat::RED_INTEGER);
        // m_frame_buffer = FrameBuffer::create(s_max_viewport_size, s_max_viewport_size, { m_frame_texture, m_entity_id_texture });

        // m_scene = CreateRef<Scene>();
        // m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
        std::ifstream in("test.yg", std::ios::in);
        if (!in) {
            YG_CORE_WARN("Could not open file '{0}'!", "scene.yg");
            return;
        }
        std::string json;
        in.seekg(0, std::ios::end);
        json.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(&json[0], json.size());
        in.close();

        m_scene = SceneManager::deserialize_scene(json);
        m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);

        Ref<Texture2D> checkboard = Texture2D::create("../sandbox/assets/textures/checkerboard.png");
        m_scene->view_components<TransformComponent, SpriteRendererComponent>([checkboard](Entity entity, TransformComponent& transform, SpriteRendererComponent& sprite){
            sprite.texture = checkboard;
        });
    }

    void EditorLayer::on_detach()
    {
        YG_PROFILE_FUNCTION();

        ImguiSetting::shutdown();
    }

    void EditorLayer::on_update(Timestep ts)
    {
        YG_PROFILE_FUNCTION();

        ImguiSetting::imgui_begin();
        
        imgui_update();
        m_hierarchy_panel->on_imgui_render();

        // Renderer2D::reset_stats();
        // m_frame_buffer->bind();

        m_editor_camera.on_update(ts, m_viewport_hovered);
        m_scene->on_update(ts);

        // m_frame_buffer->unbind();
        ImguiSetting::imgui_end();
    }

    void EditorLayer::imgui_update()
    {
        YG_PROFILE_FUNCTION();

        if (ImGui::BeginMenuBar()) {
            if (ImGui::BeginMenu("File")) {
                if (ImGui::MenuItem("New")) {
                    m_scene = CreateRef<Scene>();
                    m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
                    m_editor_camera = EditorCamera{};
                }
                if (ImGui::MenuItem("Open...")) {
                    open_scene();
                }
                if (ImGui::MenuItem("Save As...")) {
                    save_scene();
                }
                if (ImGui::MenuItem("Exit")) {
                    Application::get().close();
                }
                ImGui::EndMenu();
            }
            ImGui::EndMenuBar();
        }

        // Renderer2D::Statistics stats = Renderer2D::get_stats();
        // ImGui::Begin("Stats");
        // ImGui::Text("Renderer2D Stats:");
        // ImGui::Text("Draw Calls: %d", stats.draw_calls);
        // ImGui::Text("Quads: %d", stats.quad_count);
        // ImGui::Text("Vertices: %d", stats.get_total_vertex_count());
        // ImGui::Text("Indices: %d", stats.get_total_index_count());
        // ImGui::End();

        ImGui::Begin("Settings");
        ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        if (ImGui::TreeNodeEx("Editor Camera", flags)) {
            bool is_ortho = m_editor_camera.get_is_ortho();
            if (ImGui::Checkbox("is ortho", &is_ortho)) {
                m_editor_camera.set_is_ortho(is_ortho);
            }
            ImGui::TreePop();
        }
        ImGui::End();

        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
        ImGui::Begin("Viewport");
        m_viewport_hovered = ImGui::IsWindowHovered();
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
            m_editor_camera.on_event(e);
            RenderCommand::set_viewport(0.0f, 0.0f, m_viewport_size.x, m_viewport_size.y);
        }
        // ImGui::Image(
        //     (void*)(uint64_t)m_frame_texture->get_renderer_id(),
        //     ImVec2(m_viewport_size.x, m_viewport_size.y),
        //     ImVec2( 0, m_viewport_size.y / m_frame_texture->get_height() ),
        //     ImVec2( m_viewport_size.x / m_frame_texture->get_width(), 0 )
        // );

        // Gizmos
        Entity selected_entity = m_hierarchy_panel->get_selected_entity();
        if (selected_entity) {
            ImGuizmo::SetOrthographic(m_editor_camera.get_is_ortho());
            ImGuizmo::SetDrawlist();
            ImGuizmo::SetRect(m_viewport_bounds[0].x, m_viewport_bounds[0].y, m_viewport_size.x, m_viewport_size.y);
            
            glm::mat4 camera_projection = m_editor_camera.get_projection();
            glm::mat4 camera_view = m_editor_camera.get_view();

            bool snap = Input::is_key_pressed(YG_KEY_LEFT_ALT);
            float snap_value = 0.5f;
            if (m_gizmo_type == ImGuizmo::OPERATION::ROTATE)
                snap_value = 45.0f;

            float snap_values[3] = { snap_value, snap_value, snap_value };
            TransformComponent& tc = selected_entity.get_component<TransformComponent>();
            ImGuizmo::Manipulate(glm::value_ptr(camera_view), glm::value_ptr(camera_projection), m_gizmo_type,
                ImGuizmo::LOCAL, glm::value_ptr((glm::mat4&)tc.transform), nullptr, snap ? snap_values : nullptr);
        }

        ImGui::End();
        ImGui::PopStyleVar();
    }

    void EditorLayer::on_event(Event& e)
    {
        YG_PROFILE_FUNCTION();
        
        if (e.get_event_type() != WindowResizeEvent::get_static_type()) {
            EventDispatcher dispatcher(e);
            if (m_viewport_hovered) {
                m_editor_camera.on_event(e);
                dispatcher.dispatch<MouseButtonPressedEvent>(YG_BIND_EVENT_FN(EditorLayer::on_mouse_button_pressed));
            }
            m_scene->on_event(e);
            dispatcher.dispatch<KeyPressedEvent>(YG_BIND_EVENT_FN(EditorLayer::on_key_pressed));
        }
        else {
            e.m_handled = true;
        }
    }

    bool EditorLayer::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if (e.get_mouse_button() == YG_MOUSE_BUTTON_1 && !ImGuizmo::IsOver()) {
            int entity_id;
            ImVec2 mouse_pos = ImGui::GetMousePos();
            int32_t mouse_x = mouse_pos.x - m_viewport_bounds[0].x;
            int32_t mouse_y = mouse_pos.y - m_viewport_bounds[0].y;
            mouse_y = m_viewport_size.y - mouse_y;
            m_entity_id_texture->read_pixel(mouse_x, mouse_y, &entity_id);
            if (entity_id & 0x20000000) {
                m_hierarchy_panel->set_selected_entity(Entity{});
            }
            else {
                m_hierarchy_panel->set_selected_entity(m_scene->get_entity((uint32_t)entity_id));
            }
        }
        return false;
    }

    bool EditorLayer::on_key_pressed(KeyPressedEvent& e)
    {
        switch (e.get_key_code()) {
            case YG_KEY_W: m_gizmo_type = ImGuizmo::OPERATION::TRANSLATE; break;
            case YG_KEY_E: m_gizmo_type = ImGuizmo::OPERATION::ROTATE; break;
            case YG_KEY_R: m_gizmo_type = ImGuizmo::OPERATION::SCALE; break;
        }
        return false;
    }

    void EditorLayer::open_scene()
    {
        auto f = pfd::open_file("Open scene").result();
        if (!f.empty()) {
            std::ifstream in(f[0], std::ios::in);
            if (!in) {
                YG_CORE_WARN("Could not open file '{0}'!", "scene.yg");
                return;
            }
            std::string json;
            in.seekg(0, std::ios::end);
            json.resize(in.tellg());
            in.seekg(0, std::ios::beg);
            in.read(&json[0], json.size());
            in.close();

            m_scene = SceneManager::deserialize_scene(json);
            m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
        }
    }

    void EditorLayer::save_scene()
    {
        auto f = pfd::save_file("Save scene").result();
        if (!f.empty()) {
            std::string json = SceneManager::serialize_scene(m_scene);

            std::ofstream out(f, std::ios::out);
            if (!out) {
                YG_CORE_ERROR("Could not open file '{0}'!", "scene.yg");
                return;
            }
            out << json;
            out.close();
        }
    }

}