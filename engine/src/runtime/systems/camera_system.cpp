#include "runtime/systems/camera_system.h"
#include "runtime/scene/components.h"
#include "runtime/renderer/renderer_2d.h"

namespace Yogi {

    glm::mat4 last_camera_transform = glm::mat4(1.0f);
    glm::mat4 last_camera_transform_inverse = glm::mat4(1.0f);
    bool last_camera_ortho = false;
    float last_camera_fov = 1.0f;
    float last_camera_aspect_ratio = 1.0f;
    float last_camera_zoom_level = 1.0f;
    glm::mat4 last_camera_projection = glm::mat4(1.0f);

    void CameraSystem::on_update(Timestep ts, Scene* scene)
    {
        scene->view_components<TransformComponent, CameraComponent>([ts](Entity entity, TransformComponent& transform, CameraComponent& camera){
            if (camera.is_primary) {
                if ((glm::mat4)transform.transform != last_camera_transform) {
                    last_camera_transform = transform.transform;
                    last_camera_transform_inverse = glm::inverse(last_camera_transform);
                    Renderer2D::set_projection_view_matrix(last_camera_projection * last_camera_transform_inverse);
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
                    Renderer2D::set_projection_view_matrix(last_camera_projection * last_camera_transform_inverse);
                }
            }
        });
    }

}