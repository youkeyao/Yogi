#pragma once

#include "base/core/timestep.h"
#include "base/events/application_event.h"
#include "base/events/mouse_event.h"
#include "base/renderer/orthographic_camera.h"

namespace hazel {

    class OrthographicCameraController
    {
    public:
        OrthographicCameraController(float aspect_ratio, bool rotation = false);

        void on_update(TimeStep ts);
        void on_event(Event& e);

        OrthographicCamera& get_camera() { return m_camera; }

        float zoom_level() const { return m_zoom_level; }
        void set_zoom_level(float level) { m_zoom_level = level; }

    private:
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_window_resized(WindowResizeEvent& e);
    private:
        float m_aspect_ratio;
        float m_zoom_level = 1.0f;
        OrthographicCamera m_camera;

        glm::vec3 m_camera_position = { 0.0f, 0.0f, 0.0f };
        float m_camera_rotation = 0.0f;
        float m_camera_translation_speed = 5.0f;
        float m_camera_rotation_speed = 180.0f;

        bool m_rotation;
    };

}