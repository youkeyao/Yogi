#include "base/core/input.h"
#include "base/events/key_codes.h"
#include "base/renderer/orthographic_camera_controller.h"

namespace hazel {

    OrthographicCameraController::OrthographicCameraController(float aspect_ratio, bool rotation)
        : m_aspect_ratio(aspect_ratio),
        m_camera(-aspect_ratio * m_zoom_level, aspect_ratio * m_zoom_level, -m_zoom_level, m_zoom_level),
        m_rotation(rotation) {}

    void OrthographicCameraController::on_update(TimeStep ts)
    {
        HZ_PROFILE_FUNCTION();

        if (Input::is_key_pressed(HZ_KEY_A)) {
            m_camera_position.x -= m_camera_translation_speed * ts;
        } else if (Input::is_key_pressed(HZ_KEY_D)) {
            m_camera_position.x += m_camera_translation_speed * ts;
        }

        if (Input::is_key_pressed(HZ_KEY_W)) {
            m_camera_position.y += m_camera_translation_speed * ts;
        } else if (Input::is_key_pressed(HZ_KEY_S)) {
            m_camera_position.y -= m_camera_translation_speed * ts;
        }

        if (m_rotation) {
            if (Input::is_key_pressed(HZ_KEY_Q)) {
                m_camera_rotation += m_camera_rotation_speed * ts;
            } else if (Input::is_key_pressed(HZ_KEY_E)) {
                m_camera_rotation -= m_camera_rotation_speed * ts;
            }
            m_camera.set_rotation(m_camera_rotation);
        }

        m_camera.set_position(m_camera_position);
        
        m_camera_translation_speed = m_zoom_level;
    }

    void OrthographicCameraController::on_event(Event& e)
    {
        HZ_PROFILE_FUNCTION();

        EventDispatcher dispatcher(e);
        dispatcher.dispatch<MouseScrolledEvent>(HZ_BIND_EVENT_FN(OrthographicCameraController::on_mouse_scrolled));
        dispatcher.dispatch<WindowResizeEvent>(HZ_BIND_EVENT_FN(OrthographicCameraController::on_window_resized));
    }

    bool OrthographicCameraController::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        HZ_PROFILE_FUNCTION();

        m_zoom_level -= e.get_y_offset() * 0.25f;
        m_zoom_level = std::max(m_zoom_level, 0.25f);
        m_camera.set_projection(-m_aspect_ratio * m_zoom_level, m_aspect_ratio * m_zoom_level, -m_zoom_level, m_zoom_level);

        return false;
    }

    bool OrthographicCameraController::on_window_resized(WindowResizeEvent& e)
    {
        HZ_PROFILE_FUNCTION();
        
        m_aspect_ratio = (float)e.get_width() / (float)e.get_height();
        m_camera.set_projection(-m_aspect_ratio * m_zoom_level, m_aspect_ratio * m_zoom_level, -m_zoom_level, m_zoom_level);

        return false;
    }

}