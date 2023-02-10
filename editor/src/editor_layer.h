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
        void on_imgui_render() override;
        void on_event(Event& event) override;

    private:
        OrthographicCameraController m_camera_controller;

        Ref<Texture2D> m_checkerboard_texture;
        Ref<FrameBuffer> m_frame_buffer;

        glm::vec4 m_square_color = { 0.2f, 0.3f, 0.8f, 1.0f };
    };

}