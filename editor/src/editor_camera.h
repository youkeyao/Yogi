#pragma once

#include <engine.h>

namespace Yogi {

    class EditorCamera
    {
    public:
        void on_update(Timestep ts);
        void on_event(Event& e);

        glm::mat4 get_projection() { return m_camera_projection_matrix; }
    private:
        void recalculate_projection();
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_window_resized(WindowResizeEvent& e);
    private:
        TransformComponent m_transform_component;
        CameraComponent m_camera_component;
        glm::mat4 m_transform_inverse_matrix;
        glm::mat4 m_camera_projection_matrix;
        float m_camera_translation_speed = 1.0f;
    };

}