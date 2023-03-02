#include "base/systems/camera_system.h"
#include "base/scene/components.h"
#include "base/renderer/renderer_2d.h"

namespace Yogi {

    glm::vec3 last_camera_translation = glm::vec3(1.0f);
    glm::vec3 last_camera_rotation = glm::vec3(1.0f);
    glm::vec3 last_camera_scale = glm::vec3(1.0f);
    glm::mat4 last_camera_transform_inverse = glm::mat4(1.0f);
    bool last_camera_ortho = false;
    float last_camera_fov = 1.0f;
    float last_camera_aspect_ratio = 1.0f;
    float last_camera_zoom_level = 1.0f;
    glm::mat4 last_camera_projection = glm::mat4(1.0f);

    void CameraSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        scene->view_components<TransformComponent, CameraComponent>([ts](TransformComponent& transform, CameraComponent& camera){
            if (camera.is_primary) {
                if (transform.translation != last_camera_translation || transform.rotation != last_camera_rotation ||
                    transform.scale != last_camera_scale
                ) {
                    last_camera_translation = transform.translation;
                    last_camera_rotation = transform.rotation;
                    last_camera_scale = transform.scale;
                    glm::mat4 t = glm::translate(glm::mat4(1.0f), transform.translation) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.x), glm::vec3(1, 0, 0)) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.y), glm::vec3(0, 1, 0)) *
                        glm::rotate(glm::mat4(1.0f), glm::radians(transform.rotation.z), glm::vec3(0, 0, 1)) *
                        glm::scale(glm::mat4(1.0f), transform.scale);
                    last_camera_transform_inverse = glm::inverse(t);
                    Renderer2D::set_view_projection_matrix(last_camera_projection * last_camera_transform_inverse);
                }
                if (camera.is_ortho != last_camera_ortho || camera.fov != last_camera_fov ||
                    camera.aspect_ratio != last_camera_aspect_ratio || camera.zoom_level != last_camera_zoom_level
                ) {
                    camera.zoom_level = std::max(camera.zoom_level, 0.25f);
                    last_camera_ortho = camera.is_ortho;
                    last_camera_fov = camera.fov;
                    last_camera_aspect_ratio = camera.aspect_ratio;
                    last_camera_zoom_level = camera.zoom_level;
                    if (camera.is_ortho)
                        last_camera_projection = glm::ortho(-camera.aspect_ratio * camera.zoom_level, camera.aspect_ratio * camera.zoom_level, -camera.zoom_level, camera.zoom_level, -1.0f, 1.0f);
                    else
                        last_camera_projection = glm::perspective(camera.fov, camera.aspect_ratio, camera.zoom_level, 100.0f);
                    Renderer2D::set_view_projection_matrix(last_camera_projection * last_camera_transform_inverse);
                }
            }
        });
    }

}