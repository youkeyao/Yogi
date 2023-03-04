#include "editor_camera.h"

namespace Yogi {

    void EditorCamera::on_update(Timestep ts)
    {
        bool is_moved = false;
        if (Input::is_key_pressed(YG_KEY_A)) {
            m_transform_component.translation.x -= m_camera_translation_speed * ts;
            is_moved = true;
        } else if (Input::is_key_pressed(YG_KEY_D)) {
            m_transform_component.translation.x += m_camera_translation_speed * ts;
            is_moved = true;
        }

        if (Input::is_key_pressed(YG_KEY_W)) {
            m_transform_component.translation.y += m_camera_translation_speed * ts;
            is_moved = true;
        } else if (Input::is_key_pressed(YG_KEY_S)) {
            m_transform_component.translation.y -= m_camera_translation_speed * ts;
            is_moved = true;
        }

        if (is_moved) {
            glm::mat4 t = glm::translate(glm::mat4(1.0f), m_transform_component.translation) *
                glm::rotate(glm::mat4(1.0f), glm::radians(m_transform_component.rotation.x), glm::vec3(1, 0, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(m_transform_component.rotation.y), glm::vec3(0, 1, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(m_transform_component.rotation.z), glm::vec3(0, 0, 1)) *
                glm::scale(glm::mat4(1.0f), m_transform_component.scale);
            m_transform_inverse_matrix = glm::inverse(t);
            Renderer2D::set_view_projection_matrix(m_camera_projection_matrix * glm::inverse(t));
        }
    }

    void EditorCamera::recalculate_projection()
    {
        if (m_camera_component.is_ortho)
            m_camera_projection_matrix = glm::ortho(-m_camera_component.aspect_ratio * m_camera_component.zoom_level, m_camera_component.aspect_ratio * m_camera_component.zoom_level, -m_camera_component.zoom_level, m_camera_component.zoom_level, -1.0f, 1.0f);
        else
            m_camera_projection_matrix = glm::perspective(m_camera_component.fov, m_camera_component.aspect_ratio, m_camera_component.zoom_level, 100.0f);
        Renderer2D::set_view_projection_matrix(m_camera_projection_matrix * m_transform_inverse_matrix);
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