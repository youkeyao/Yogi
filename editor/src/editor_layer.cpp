#include "editor_layer.h"
#include "imgui_setting.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"
#include "reflect/scene_manager.h"
#include <glm/gtc/type_ptr.hpp>
#include <portable-file-dialogs.h>
#include "panels/fontawesome4_header.h"

namespace Yogi {

    EditorLayer::EditorLayer() : Layer("EditorLayer") {}

    void EditorLayer::on_attach()
    {
        YG_PROFILE_FUNCTION();

        ImguiSetting::init();

        ComponentManager::init();
        SystemManager::init();

        m_frame_texture = RenderTexture::create("frame_texture", 1, 1, TextureFormat::ATTACHMENT);
        m_entity_id_texture = RenderTexture::create("entity_id", 1, 1, TextureFormat::RED_INTEGER);
        m_frame_buffer = FrameBuffer::create(1, 1, { m_frame_texture });
        m_entity_frame_buffer = FrameBuffer::create(1, 1, { m_entity_id_texture });
        m_entity_id_mat = Material::create("entity_id", PipelineManager::get_pipeline("Entity"));

        m_editor_render_system = CreateRef<RenderSystem>();
        RenderSystem::set_default_frame_buffer(m_frame_buffer);

        AssetManager::init_project(YG_PROJECT_TEMPLATE);
        m_scene = CreateRef<Scene>();
        m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
        m_content_browser_panel = CreateRef<ContentBrowserPanel>(YG_PROJECT_TEMPLATE);
        m_material_editor_panel = CreateRef<MaterialEditorPanel>();
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
        m_content_browser_panel->on_imgui_render();
        m_material_editor_panel->on_imgui_render();

        // Edit Mode
        if (m_scene_state == SceneState::Edit) {
            Renderer::reset_stats();
            m_editor_camera.on_update(ts, m_viewport_hovered);
            m_editor_render_system->set_light(m_scene.get());
            m_editor_render_system->render_camera(m_editor_camera.get_camera_component(), m_editor_camera.get_transform_component(), m_scene.get());
            // Entity id
            m_entity_frame_buffer->bind();
            RenderCommand::clear();
            m_scene->view_components<TransformComponent, MeshRendererComponent>([&](Entity entity, TransformComponent& transform, MeshRendererComponent& mesh_renderer){
                TransformComponent tmp_transform = transform;
                glm::mat4 transform_mat = transform.transform;
                while (tmp_transform.parent) {
                    tmp_transform = tmp_transform.parent.get_component<TransformComponent>();
                    transform_mat = (glm::mat4)tmp_transform.transform * transform_mat;
                }
                Renderer::draw_mesh(mesh_renderer.mesh, m_entity_id_mat, transform_mat, entity);
            });
            Renderer::flush();
            m_entity_frame_buffer->unbind();
        }
        // Play Mode
        else {
            m_scene->on_update(ts);
        }

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
                if (ImGui::MenuItem("Open Project...")) {
                    open_project();
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

        Renderer::Statistics stats = Renderer::get_stats();
        ImGui::Begin("Stats");
        ImGui::Text("Draw Calls: %d", stats.draw_calls);
        ImGui::Text("Vertices: %d", stats.vertices_count);
        ImGui::Text("Indices: %d", stats.indices_count);
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
        if ((uint32_t)new_viewport_size.x < 0 || (uint32_t)new_viewport_size.y < 0) {
            YG_CORE_WARN("Invalid viewport size!");
        }
        else if (new_viewport_size.x != m_viewport_size.x || new_viewport_size.y != m_viewport_size.y) {
            m_viewport_size = new_viewport_size;
            WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y, nullptr);
            on_window_resized(e);
        }
        ImguiSetting::show_image(
            m_frame_texture,
            ImVec2(m_viewport_size.x, m_viewport_size.y),
            ImVec2(1, 1)
        );

        // Drop scene
        if (ImGui::BeginDragDropTarget()) {
            ImGui::GetWindowDrawList()->AddRect(
                { viewport_region_min.x + viewport_offset.x + 1.0f, viewport_region_min.y + viewport_offset.y + 1.0f },
                { viewport_region_max.x + viewport_offset.x - 1.0f, viewport_region_max.y + viewport_offset.y - 1.0f },
                ImGui::GetColorU32(ImGuiCol_DragDropTarget), 0.0f, 0, 2.0f
            );
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("content_browser_item", ImGuiDragDropFlags_AcceptNoDrawDefaultRect)) {
                const char* path = (const char*)payload->Data;
                std::filesystem::path filepath = path;
                auto t = filepath.extension().string();
                if (filepath.extension().string() == ".yg")
                    open_scene(std::string(path));
            }
            ImGui::EndDragDropTarget();
        }

        // Gizmos
        Entity selected_entity = m_hierarchy_panel->get_selected_entity();
        if (selected_entity && m_scene_state == SceneState::Edit) {
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

        toolbar_update();
        ImGui::PopStyleVar();

        scene_info_update();
    }

    void EditorLayer::scene_info_update()
    {
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
    }

    void EditorLayer::toolbar_update()
    {
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

        ImGui::Begin("Toolbar", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
        float size = ImGui::GetWindowHeight() - 4;
        ImGui::SetWindowFontScale(size / 96);

        const char* icon = m_scene_state == SceneState::Edit ? ICON_FA_PLAY : ICON_FA_STOP;
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
        if (ImGui::Button("##playbutton", ImVec2(size, size))) {
            if (m_scene_state == SceneState::Edit) {
                WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y, nullptr);
                m_scene->on_event(e);
                m_scene_state = SceneState::Play;
            }
            else {
                WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y, nullptr);
                m_editor_camera.on_event(e);
                m_scene_state = SceneState::Edit;
            }
        }
        auto old_pos = ImGui::GetCursorPos();
        auto icon_size = ImGui::CalcTextSize(icon);
        ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - icon_size.x / 2 + 1);
        ImGui::SetCursorPosY(old_pos.y - size / 2 + icon_size.y / 2);
        ImGui::Text("%s", icon);
        ImGui::SetCursorPos(old_pos);

        ImGui::End();
        ImGui::PopStyleVar(3);
    }

    void EditorLayer::on_event(Event& e)
    {
        YG_PROFILE_FUNCTION();
        
        ImguiSetting::imgui_on_event(e);
        if (e.get_event_type() != WindowResizeEvent::get_static_type()) {
            EventDispatcher dispatcher(e);
            if (m_viewport_hovered && m_scene_state == SceneState::Edit) {
                dispatcher.dispatch<MouseButtonPressedEvent>(YG_BIND_EVENT_FN(EditorLayer::on_mouse_button_pressed));
                dispatcher.dispatch<KeyPressedEvent>(YG_BIND_EVENT_FN(EditorLayer::on_key_pressed));
            }
            if (m_scene_state == SceneState::Edit) {
                m_editor_camera.on_event(e);
                m_editor_render_system->on_event(e, m_scene.get());
            }
            else {
                m_scene->on_event(e);
            }
        }
        else {
            e.m_handled = true;
        }
    }

    void EditorLayer::on_window_resized(WindowResizeEvent& e)
    {
        if (m_scene_state == SceneState::Edit) {
            m_editor_camera.on_event(e);
            m_editor_render_system->on_event(e, m_scene.get());
        }
        else {
            m_scene->on_event(e);
        }
        m_entity_frame_buffer->resize(e.get_width(), e.get_height());
    }

    bool EditorLayer::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if (e.get_mouse_button() == YG_MOUSE_BUTTON_LEFT && (!m_hierarchy_panel->get_selected_entity() || !ImGuizmo::IsOver())) {
            int entity_id;
            ImVec2 mouse_pos = ImGui::GetMousePos();
            int32_t mouse_x = mouse_pos.x - m_viewport_bounds[0].x;
            int32_t mouse_y = mouse_pos.y - m_viewport_bounds[0].y;
            if (mouse_y >= 0 && mouse_x >= 0) {
                mouse_y = m_viewport_size.y - mouse_y;
                m_entity_id_texture->read_pixel(mouse_x, mouse_y, &entity_id);
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

    void EditorLayer::open_project()
    {
        auto f = pfd::select_folder("Open project").result();
        if (!f.empty()) {
            AssetManager::init_project(f);
            m_scene = CreateRef<Scene>();
            m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
            m_content_browser_panel = CreateRef<ContentBrowserPanel>(f);
            open_scene(f + "/main.yg");
        }
    }

    void EditorLayer::open_scene(const std::string& path)
    {
        std::ifstream in(path, std::ios::in);
        if (!in) {
            YG_CORE_WARN("Could not open file '{0}'!", path);
            return;
        }
        std::string json;
        in.seekg(0, std::ios::end);
        json.resize(in.tellg());
        in.seekg(0, std::ios::beg);
        in.read(json.data(), json.size());
        in.close();

        if (SceneManager::is_scene(json)) {
            m_scene = SceneManager::deserialize_scene(json);
            m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
        }
    }

    void EditorLayer::save_scene()
    {
        auto f = pfd::save_file("Save scene", m_content_browser_panel->get_base_dir()).result();
        if (!f.empty()) {
            std::string json = SceneManager::serialize_scene(m_scene);

            std::ofstream out(f, std::ios::out);
            if (!out) {
                YG_CORE_ERROR("Could not save scene!");
                return;
            }
            out << json;
            out.close();
        }
    }

}