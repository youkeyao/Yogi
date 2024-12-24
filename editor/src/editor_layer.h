#pragma once

#include "editor_camera.h"
#include "panels/content_browser_panel.h"
#include "panels/material_editor_panel.h"
#include "panels/scene_hierarchy_panel.h"

#include <engine.h>
#include <imgui.h>
#include <ImGuizmo.h>

namespace Yogi {

class EditorLayer : public Layer
{
public:
    EditorLayer();
    ~EditorLayer() = default;

    void on_attach() override;
    void on_detach() override;
    void on_update(Timestep ts) override;
    void on_event(Event &event) override;

    void imgui_update();
    void open_project();
    void open_scene(const std::string &path);
    void save_scene();

private:
    void scene_info_update();
    void toolbar_update();
    void on_window_resized(WindowResizeEvent &e);
    bool on_mouse_button_pressed(MouseButtonPressedEvent &e);
    bool on_key_pressed(KeyPressedEvent &e);

private:
    Ref<Scene>               m_scene;
    Ref<SceneHierarchyPanel> m_hierarchy_panel;
    Ref<ContentBrowserPanel> m_content_browser_panel;
    Ref<MaterialEditorPanel> m_material_editor_panel;

    Ref<RenderSystem> m_editor_render_system;
    EditorCamera      m_editor_camera;

    bool                m_viewport_hovered = false;
    glm::vec2           m_viewport_size;
    glm::vec2           m_viewport_bounds[2];
    ImGuizmo::OPERATION m_gizmo_type = ImGuizmo::OPERATION::TRANSLATE;

    Ref<RenderTexture> m_frame_texture;
    Ref<RenderTexture> m_entity_id_texture;
    Ref<FrameBuffer>   m_frame_buffer;
    Ref<FrameBuffer>   m_entity_frame_buffer;
    Ref<Material>      m_entity_id_mat;

    enum class SceneState { Edit = 0, Play = 1 };
    SceneState m_scene_state = SceneState::Edit;
};

}  // namespace Yogi