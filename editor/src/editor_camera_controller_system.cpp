#include "editor_camera_controller_system.h"

namespace Yogi {

    float s_camera_translation_speed = 1.0f;
    float s_camera_rotation_speed = 1.0f;

    void EditorCameraControllerSystem::on_update(Timestep ts, Scene* scene)
    {
        scene->view_components<TransformComponent, CameraComponent>([ts](TransformComponent& transform, CameraComponent& camera){
            if (camera.is_primary) {
                if (Input::is_key_pressed(YG_KEY_A)) {
                    transform.translation.x -= s_camera_translation_speed * ts;
                } else if (Input::is_key_pressed(YG_KEY_D)) {
                    transform.translation.x += s_camera_translation_speed * ts;
                }

                if (Input::is_key_pressed(YG_KEY_W)) {
                    transform.translation.y += s_camera_translation_speed * ts;
                } else if (Input::is_key_pressed(YG_KEY_S)) {
                    transform.translation.y -= s_camera_translation_speed * ts;
                }

                if (Input::is_key_pressed(YG_KEY_Q)) {
                    transform.rotation.z += s_camera_rotation_speed * ts;
                } else if (Input::is_key_pressed(YG_KEY_E)) {
                    transform.rotation.z -= s_camera_rotation_speed * ts;
                }
            }
        });
    }

    void EditorCameraControllerSystem::on_event(Event& e, Scene* scene)
    {
        EventDispatcher dispatcher(e);
        dispatcher.dispatch<MouseScrolledEvent>(EditorCameraControllerSystem::on_mouse_scrolled, scene);
        dispatcher.dispatch<WindowResizeEvent>(EditorCameraControllerSystem::on_window_resized, scene);
    }

    bool EditorCameraControllerSystem::on_mouse_scrolled(MouseScrolledEvent& e, Scene* scene)
    {
        scene->view_components<CameraComponent>([e](CameraComponent& camera){
            if (camera.is_primary) {
                camera.zoom_level -= e.get_y_offset() * 0.25f;
                s_camera_translation_speed = camera.zoom_level;
            }
        });

        return false;
    }

    bool EditorCameraControllerSystem::on_window_resized(WindowResizeEvent& e, Scene* scene)
    {
        scene->view_components<CameraComponent>([e](CameraComponent& camera){
            if (camera.is_primary) {
                camera.aspect_ratio = (float)e.get_width() / (float)e.get_height();
            }
        });

        return false;
    }

}