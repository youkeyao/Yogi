#include "editor_camera.h"

namespace Yogi {

    EditorCamera::EditorCamera()
    {
        m_camera_view_matrix = glm::inverse((glm::mat4)m_transform_component.transform);
        recalculate_projection();
        m_camera_projection_view_matrix = m_camera_projection_matrix * m_camera_view_matrix;
    }

    void EditorCamera::on_update(Timestep ts, bool is_focused)
    {
        if (is_focused) {
            bool is_moved = false;
            glm::mat4& transform = (glm::mat4&)m_transform_component.transform;
            if (Input::is_key_pressed(YG_KEY_A)) {
                transform = glm::translate(transform, {-m_camera_translation_speed * ts, 0, 0});
                is_moved = true;
            } else if (Input::is_key_pressed(YG_KEY_D)) {
                transform = glm::translate(transform, {m_camera_translation_speed * ts, 0, 0});
                is_moved = true;
            }

            if (Input::is_key_pressed(YG_KEY_W)) {
                transform = glm::translate(transform, {0, m_camera_translation_speed * ts, 0});
                is_moved = true;
            } else if (Input::is_key_pressed(YG_KEY_S)) {
                transform = glm::translate(transform, {0, -m_camera_translation_speed * ts, 0});
                is_moved = true;
            }

            if (is_moved) {
                m_camera_view_matrix = glm::inverse(transform);
                m_camera_projection_view_matrix = m_camera_projection_matrix * m_camera_view_matrix;
            }
        }
        
        Renderer2D::set_projection_view_matrix(m_camera_projection_view_matrix);
    }

    void EditorCamera::recalculate_projection()
    {
        if (m_camera_component.is_ortho)
            m_camera_projection_matrix = glm::ortho(-m_camera_component.aspect_ratio * m_camera_component.zoom_level, m_camera_component.aspect_ratio * m_camera_component.zoom_level, -m_camera_component.zoom_level, m_camera_component.zoom_level, -1.0f, 1.0f);
        else
            m_camera_projection_matrix = glm::perspective(m_camera_component.fov, m_camera_component.aspect_ratio, m_camera_component.zoom_level, 100.0f);
        m_camera_projection_view_matrix = m_camera_projection_matrix * m_camera_view_matrix;
    }

    void EditorCamera::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<MouseScrolledEvent>(YG_BIND_EVENT_FN(EditorCamera::on_mouse_scrolled));
        dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(EditorCamera::on_window_resized));
    }

    bool EditorCamera::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        m_camera_component.zoom_level -= e.get_y_offset() * 0.25f;
        m_camera_component.zoom_level = std::max(m_camera_component.zoom_level, 0.25f);
        m_camera_translation_speed = m_camera_component.zoom_level;
        recalculate_projection();

        return false;
    }

    bool EditorCamera::on_window_resized(WindowResizeEvent& e)
    {
        m_camera_component.aspect_ratio = (float)e.get_width() / (float)e.get_height();
        recalculate_projection();

        return false;
    }

}