#pragma once

#include <engine.h>
#include <imgui.h>
#include <ImGuizmo.h>
#include "editor_camera.h"
#include "panels/scene_hierarchy_panel.h"
#include "panels/content_browser_panel.h"
#include "panels/material_editor_panel.h"

namespace Yogi {

    class EditorLayer : public Layer
    {
    public:
        EditorLayer();
        ~EditorLayer() = default;

        void on_attach() override;
        void on_detach() override;
        void on_update(Timestep ts) override;
        void on_event(Event& event) override;

        void imgui_update();
        void open_project();
        void open_scene(const std::string& path);
        void save_scene();
    private:
        void toolbar_update();
        bool on_mouse_button_pressed(MouseButtonPressedEvent& e);
        bool on_key_pressed(KeyPressedEvent& e);
    private:
        Ref<Scene> m_scene;
        Ref<SceneHierarchyPanel> m_hierarchy_panel;
        Ref<ContentBrowserPanel> m_content_browser_panel;
        Ref<MaterialEditorPanel> m_material_editor_panel;

        EditorCamera m_editor_camera;

        bool m_viewport_hovered = false;
        glm::vec2 m_viewport_size;
        glm::vec2 m_viewport_bounds[2];
        ImGuizmo::OPERATION m_gizmo_type = ImGuizmo::OPERATION::TRANSLATE;

        Ref<Texture2D> m_frame_texture;
        Ref<Texture2D> m_entity_id_texture;
        Ref<FrameBuffer> m_frame_buffer;

        enum class SceneState
		{
			Edit = 0, Play = 1
		};
		SceneState m_scene_state = SceneState::Edit;
    };

}