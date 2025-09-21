#include "Layers/ViewportLayer.h"

#include <imgui.h>
#include "Utils/fontawesome4_header.h"

namespace Yogi
{

ViewportLayer::ViewportLayer(Handle<World>& world, Entity& selectedEntity) :
    Layer("Viewport Layer"),
    m_world(world),
    m_selectedEntity(selectedEntity)
{}

void ViewportLayer::OnUpdate(Timestep ts)
{
    OnGUI();
    m_world->OnUpdate(ts);
}

void ViewportLayer::OnEvent(Event& event) { m_world->OnEvent(event); }

void ViewportLayer::OnGUI()
{
    if (ImGui::BeginMenuBar())
    {
        if (ImGui::BeginMenu("File"))
        {
            if (ImGui::MenuItem("New"))
            {
                m_world = Handle<World>::Create();
                // m_hierarchy_panel = CreateRef<SceneHierarchyPanel>(m_scene);
                // m_editor_camera   = EditorCamera{};
            }
            if (ImGui::MenuItem("Open Project..."))
            {
                // open_project();
            }
            if (ImGui::MenuItem("Save As..."))
            {
                // save_scene();
            }
            if (ImGui::MenuItem("Exit"))
            {
                Application::GetInstance().Close();
            }
            ImGui::EndMenu();
        }
        ImGui::EndMenuBar();
    }

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("Viewport");
    m_viewportHovered           = ImGui::IsWindowHovered();
    ImVec2 viewportRegionMin    = ImGui::GetWindowContentRegionMin();
    ImVec2 viewportRegionMax    = ImGui::GetWindowContentRegionMax();
    ImVec2 viewportOffset       = ImGui::GetWindowPos();
    m_viewportBounds[0]         = { viewportRegionMin.x + viewportOffset.x, viewportRegionMin.y + viewportOffset.y };
    m_viewportBounds[1]         = { viewportRegionMax.x + viewportOffset.x, viewportRegionMax.y + viewportOffset.y };
    glm::vec2 new_viewport_size = { viewportRegionMax.x - viewportRegionMin.x,
                                    viewportRegionMax.y - viewportRegionMin.y };
    if ((uint32_t)new_viewport_size.x < 0 || (uint32_t)new_viewport_size.y < 0)
    {
        YG_CORE_WARN("Invalid viewport size!");
    }
    else if (new_viewport_size.x != m_viewportSize.x || new_viewport_size.y != m_viewportSize.y)
    {
        m_viewportSize = new_viewport_size;
        WindowResizeEvent e((uint32_t)m_viewportSize.x, (uint32_t)m_viewportSize.y, nullptr);
        // on_window_resized(e);
    }
    // ImguiSetting::show_image(m_frame_texture, ImVec2(m_viewportSize.x, m_viewportSize.y), ImVec2(1, 1));

    // Drop scene
    if (ImGui::BeginDragDropTarget())
    {
        ImGui::GetWindowDrawList()->AddRect(
            { viewportRegionMin.x + viewportOffset.x + 1.0f, viewportRegionMin.y + viewportOffset.y + 1.0f },
            { viewportRegionMax.x + viewportOffset.x - 1.0f, viewportRegionMax.y + viewportOffset.y - 1.0f },
            ImGui::GetColorU32(ImGuiCol_DragDropTarget),
            0.0f,
            0,
            2.0f);
        // if (const ImGuiPayload* payload =
        //         ImGui::AcceptDragDropPayload("content_browser_item", ImGuiDragDropFlags_AcceptNoDrawDefaultRect))
        // {
        //     const char*           path     = (const char*)payload->Data;
        //     std::filesystem::path filepath = path;
        //     auto                  t        = filepath.extension().string();
        //     if (filepath.extension().string() == ".yg")
        //         open_scene(std::string(path));
        // }
        ImGui::EndDragDropTarget();
    }

    // Gizmos
    // Entity selected_entity = m_hierarchy_panel->get_selected_entity();
    // if (selected_entity && m_sceneState == SceneState::Edit)
    // {
    //     ImGuizmo::SetOrthographic(m_editor_camera.get_is_ortho());
    //     ImGuizmo::SetDrawlist();
    //     ImGuizmo::SetRect(m_viewport_bounds[0].x, m_viewport_bounds[0].y, m_viewportSize.x, m_viewportSize.y);

    //     glm::mat4 camera_projection = m_editor_camera.get_projection();
    //     glm::mat4 camera_view       = m_editor_camera.get_view();

    //     bool  snap       = Input::is_key_pressed(YG_KEY_LEFT_ALT);
    //     float snap_value = 0.5f;
    //     if (m_gizmo_type == ImGuizmo::OPERATION::ROTATE)
    //         snap_value = 45.0f;

    //     float               snap_values[3] = { snap_value, snap_value, snap_value };
    //     TransformComponent& tc             = selected_entity.get_component<TransformComponent>();
    //     ImGuizmo::Manipulate(glm::value_ptr(camera_view),
    //                          glm::value_ptr(camera_projection),
    //                          m_gizmo_type,
    //                          ImGuizmo::LOCAL,
    //                          glm::value_ptr((glm::mat4&)tc.transform),
    //                          nullptr,
    //                          snap ? snap_values : nullptr);
    // }
    ImGui::End();

    OnToolbar();
    ImGui::PopStyleVar();

    // scene_info_update();
}

void ViewportLayer::OnToolbar()
{
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 2));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemInnerSpacing, ImVec2(0, 0));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

    ImGui::Begin("Toolbar",
                 nullptr,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse);
    float size = ImGui::GetWindowHeight() - 4;
    ImGui::SetWindowFontScale(size / 96);

    const char* icon = m_sceneState == SceneState::Edit ? ICON_FA_PLAY : ICON_FA_STOP;
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - (size * 0.5f));
    if (ImGui::Button("##playbutton", ImVec2(size, size)))
    {
        // if (m_sceneState == SceneState::Edit)
        // {
        //     WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y, nullptr);
        //     m_scene->on_event(e);
        //     m_sceneState = SceneState::Play;
        // }
        // else
        // {
        //     WindowResizeEvent e((uint32_t)m_viewport_size.x, (uint32_t)m_viewport_size.y, nullptr);
        //     m_editor_camera.on_event(e);
        //     m_sceneState = SceneState::Edit;
        // }
    }
    auto old_pos   = ImGui::GetCursorPos();
    auto icon_size = ImGui::CalcTextSize(icon);
    ImGui::SetCursorPosX((ImGui::GetWindowContentRegionMax().x * 0.5f) - icon_size.x / 2 + 1);
    ImGui::SetCursorPosY(old_pos.y - size / 2 + icon_size.y / 2);
    ImGui::Text("%s", icon);
    ImGui::SetCursorPos(old_pos);

    ImGui::End();
    ImGui::PopStyleVar(3);
}

} // namespace Yogi