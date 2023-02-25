#include "base/renderer/camera_system.h"
#include "base/scene/component_manager.h"
#include "base/renderer/renderer_2d.h"

namespace Yogi {

    void CameraSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        // scene->view_components<TransformComponent, CameraComponent>([ts](TransformComponent& transform, CameraComponent& camera){
        //     if (camera.is_primary) {
        //         Renderer2D::set_view_projection_matrix(camera.projection_matrix * transform.transform_inverse);
        //     }
        // });
    }

}