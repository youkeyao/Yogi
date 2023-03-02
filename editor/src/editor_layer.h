#pragma once

#include <engine.h>
#include "panels/scene_hierarchy_panel.h"

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

        void imgui_init();
        void imgui_begin();
        void imgui_end();

    private:
        Ref<Scene> m_scene;
        Ref<SceneHierarchyPanel> m_hierarchy_panel;
        Ref<Entity> m_editor_camera;

        bool m_viewport_focused = false;
        glm::vec2 m_viewport_size;
        glm::vec2 m_viewport_bounds[2];

        Ref<Texture2D> m_frame_texture;
        Ref<FrameBuffer> m_frame_buffer;

        glm::vec4 m_square_color = { 0.2f, 0.3f, 0.8f, 1.0f };
    };

}