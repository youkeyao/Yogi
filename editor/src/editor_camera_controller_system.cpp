#include "editor_camera_controller_system.h"

namespace Yogi {

    // static float s_camera_translation_speed = 5.0f;
    // static float s_camera_rotation_speed = 5.0f;

    // void EditorCameraControllerSystem::on_update(Timestep ts, Ref<Entity> editor_camera)
    // {
    //     bool is_moved = false;
    //     TransformComponent& transform = editor_camera->get_component<TransformComponent>();
    //     if (Input::is_key_pressed(YG_KEY_A)) {
    //         is_moved = true;
    //         transform.translation.x -= s_camera_translation_speed * ts;
    //     } else if (Input::is_key_pressed(YG_KEY_D)) {
    //         is_moved = true;
    //         transform.translation.x += s_camera_translation_speed * ts;
    //     }

    //     if (Input::is_key_pressed(YG_KEY_W)) {
    //         is_moved = true;
    //         transform.translation.y += s_camera_translation_speed * ts;
    //     } else if (Input::is_key_pressed(YG_KEY_S)) {
    //         is_moved = true;
    //         transform.translation.y -= s_camera_translation_speed * ts;
    //     }

    //     if (Input::is_key_pressed(YG_KEY_Q)) {
    //         is_moved = true;
    //         transform.rotation.z += s_camera_rotation_speed * ts;
    //     } else if (Input::is_key_pressed(YG_KEY_E)) {
    //         is_moved = true;
    //         transform.rotation.z -= s_camera_rotation_speed * ts;
    //     }

    //     if (is_moved) {
    //         glm::mat4 t = glm::translate(glm::mat4(1.0f), transform.translation) *
    //             glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0)) *
    //             glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.y), glm::vec3(0, 1, 0)) *
    //             glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.z), glm::vec3(0, 0, 1));
    //         transform.transform_inverse = glm::inverse(t);
    //     }
    // }

    // void EditorCameraControllerSystem::on_event(Event& e, Ref<Entity> editor_camera)
    // {
    //     EventDispatcher dispatcher(e);
    //     dispatcher.dispatch<MouseScrolledEvent>(EditorCameraControllerSystem::on_mouse_scrolled, editor_camera);
    //     dispatcher.dispatch<WindowResizeEvent>(EditorCameraControllerSystem::on_window_resized, editor_camera);
    // }

    // bool EditorCameraControllerSystem::on_mouse_scrolled(MouseScrolledEvent& e, Ref<Entity> editor_camera)
    // {
    //     CameraComponent& camera = editor_camera->get_component<CameraComponent>();
    //     camera.zoom_level -= e.get_y_offset() * 0.25f;
    //     camera.zoom_level = std::max(camera.zoom_level, 0.25f);
    //     s_camera_translation_speed = camera.zoom_level;
    //     camera.projection_matrix = glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f);

    //     return false;
    // }

    // bool EditorCameraControllerSystem::on_window_resized(WindowResizeEvent& e, Ref<Entity> editor_camera)
    // {
    //     CameraComponent& camera = editor_camera->get_component<CameraComponent>();
    //     camera.aspect_ratio = (float)e.get_width() / (float)e.get_height();
    //     camera.projection_matrix = glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f);

    //     return false;
    // }

}