#pragma once

#include <engine.h>

namespace Yogi {

    class EditorCamera
    {
    public:
        EditorCamera();
        void on_update(Timestep ts, bool is_hovered);
        void on_event(Event& e);

        bool get_is_ortho() { return m_camera_component.is_ortho; }
        void set_is_ortho(bool is_ortho) { m_camera_component.is_ortho = is_ortho; recalculate_projection(); recalculate_view(); }
        void set_render_target(const Ref<RenderTexture>& texture) { m_camera_component.render_target = texture; }
        
        const TransformComponent& get_transform_component() { return m_transform_component; }
        const CameraComponent& get_camera_component() { return m_camera_component; }
        glm::mat4 get_view() { return m_camera_view_matrix; }
        glm::mat4 get_projection() { return m_camera_projection_matrix; }
    private:
        void recalculate_projection();
        void recalculate_view();
        bool on_mouse_button_released(MouseButtonReleasedEvent& e);
        bool on_mouse_moved(MouseMovedEvent& e);
        bool on_mouse_scrolled(MouseScrolledEvent& e);
        bool on_window_resized(WindowResizeEvent& e);
    private:
        TransformComponent m_transform_component;
        CameraComponent m_camera_component;
        glm::mat4 m_camera_view_matrix = glm::mat4(1.0f);
        glm::mat4 m_camera_projection_matrix = glm::mat4(1.0f);
        glm::mat4 m_camera_projection_view_matrix = glm::mat4(1.0f);
        float m_camera_translation_speed = 1.0f;
        uint32_t m_last_mouse_x = 0;
        uint32_t m_last_mouse_y = 0;
        float m_pixel_ratio = 1.0f;
        bool is_update = false;
    };

}