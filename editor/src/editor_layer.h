#pragma once

#include <engine.h>

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

        void imgui_begin();
        void imgui_end();

    private:
        OrthographicCameraController m_camera_controller;

        bool m_viewport_focused = false;
        glm::vec2 m_viewport_size;
        glm::vec2 m_viewport_bounds[2];

        Ref<Texture2D> m_checkerboard_texture;
        Ref<Texture2D> m_frame_texture;
        Ref<Texture2D> m_frame_depth_texture;
        Ref<FrameBuffer> m_frame_buffer;

        glm::vec4 m_square_color = { 0.2f, 0.3f, 0.8f, 1.0f };
    };

}