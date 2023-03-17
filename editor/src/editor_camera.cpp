#include "editor_camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>

namespace Yogi {

    EditorCamera::EditorCamera()
    {
    }

    void EditorCamera::on_update(Timestep ts, bool is_update)
    {
        if (is_update && Input::is_mouse_button_pressed(YG_MOUSE_BUTTON_2)) {
            ImGui::SetMouseCursor(ImGuiMouseCursor_ResizeAll);
            if (!m_camera_component.is_ortho) {
                glm::mat4& transform = (glm::mat4&)m_transform_component.transform;
                if (Input::is_key_pressed(YG_KEY_A)) {
                    transform *= glm::translate(glm::mat4(1.0f), {-m_camera_translation_speed * ts, 0, 0});
                } else if (Input::is_key_pressed(YG_KEY_D)) {
                    transform *= glm::translate(glm::mat4(1.0f), {m_camera_translation_speed * ts, 0, 0});
                }

                if (Input::is_key_pressed(YG_KEY_W)) {
                    transform *= glm::translate(glm::mat4(1.0f), {0, 0, -m_camera_translation_speed * ts});
                } else if (Input::is_key_pressed(YG_KEY_S)) {
                    transform *= glm::translate(glm::mat4(1.0f), {0, 0, m_camera_translation_speed * ts});
                }
                recalculate_view();
            }
        }
    }

    void EditorCamera::recalculate_projection()
    {
        if (m_camera_component.is_ortho)
            m_camera_projection_matrix = glm::ortho(-m_camera_component.aspect_ratio * m_camera_component.zoom_level, m_camera_component.aspect_ratio * m_camera_component.zoom_level, -m_camera_component.zoom_level, m_camera_component.zoom_level, -1.0f, 1.0f);
        else
            m_camera_projection_matrix = glm::perspective(m_camera_component.fov, m_camera_component.aspect_ratio, 0.1f, 100.0f);
        m_camera_projection_view_matrix = m_camera_projection_matrix * m_camera_view_matrix;
        Renderer::set_projection_view_matrix(m_camera_projection_view_matrix);
    }

    void EditorCamera::recalculate_view()
    {
        glm::mat4& transform = (glm::mat4&)m_transform_component.transform;
        if (m_camera_component.is_ortho) {
            m_camera_view_matrix = glm::translate(glm::mat4(1.0f), glm::vec3{ -transform[3][0], -transform[3][1], 0 });
        }
        else {
            m_camera_view_matrix = glm::inverse(transform);
        }
        m_camera_projection_view_matrix = m_camera_projection_matrix * m_camera_view_matrix;
        Renderer::set_projection_view_matrix(m_camera_projection_view_matrix);
    }

    void EditorCamera::on_event(Event& e)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<MouseButtonPressedEvent>(YG_BIND_EVENT_FN(EditorCamera::on_mouse_button_pressed));
        dispatcher.dispatch<MouseMovedEvent>(YG_BIND_EVENT_FN(EditorCamera::on_mouse_moved));
        dispatcher.dispatch<MouseScrolledEvent>(YG_BIND_EVENT_FN(EditorCamera::on_mouse_scrolled));
        dispatcher.dispatch<WindowResizeEvent>(YG_BIND_EVENT_FN(EditorCamera::on_window_resized));
    }

    bool EditorCamera::on_mouse_button_pressed(MouseButtonPressedEvent& e)
    {
        if (e.get_mouse_button() == YG_MOUSE_BUTTON_2) {
            m_last_mouse_x = Input::get_mouse_x();
            m_last_mouse_y = Input::get_mouse_y();
        }
        return false;
    }
    bool EditorCamera::on_mouse_moved(MouseMovedEvent& e)
    {
        if (Input::is_mouse_button_pressed(YG_MOUSE_BUTTON_2)) {
            glm::mat4& transform = (glm::mat4&)m_transform_component.transform;
            if (m_camera_component.is_ortho) {
                transform = glm::translate(transform, {(m_last_mouse_x - e.get_x()) / m_pixel_ratio, (e.get_y() - m_last_mouse_y) / m_pixel_ratio, 0});
            }
            else {
                transform *= glm::rotate(glm::mat4(1.0f), glm::radians(m_last_mouse_y - e.get_y()) / m_pixel_ratio * 40.0f, glm::vec3{1.0f, 0, 0});
                transform *= glm::rotate(glm::mat4(1.0f), glm::radians(m_last_mouse_x - e.get_x()) / m_pixel_ratio * 40.0f, glm::inverse(glm::mat3(transform)) * glm::vec3{0, 1.0f, 0});
            }
            recalculate_view();
            m_last_mouse_x = e.get_x();
            m_last_mouse_y = e.get_y();
        }

        return false;
    }

    bool EditorCamera::on_mouse_scrolled(MouseScrolledEvent& e)
    {
        if (m_camera_component.is_ortho) {
            m_pixel_ratio *= m_camera_component.zoom_level;
            m_camera_component.zoom_level -= e.get_y_offset() * 0.25f;
            m_camera_component.zoom_level = std::max(m_camera_component.zoom_level, 0.25f);
            m_camera_translation_speed = m_camera_component.zoom_level;
            m_pixel_ratio /= m_camera_component.zoom_level;
            recalculate_projection();
        }
        else {
            glm::mat4& transform = (glm::mat4&)m_transform_component.transform;
            transform *= glm::translate(glm::mat4(1.0f), {0, 0, -e.get_y_offset()});
            recalculate_view();
        }

        return false;
    }

    bool EditorCamera::on_window_resized(WindowResizeEvent& e)
    {
        m_camera_component.aspect_ratio = (float)e.get_width() / (float)e.get_height();
        m_pixel_ratio = e.get_height() * 0.5 / m_camera_component.zoom_level;
        recalculate_projection();

        return false;
    }

}