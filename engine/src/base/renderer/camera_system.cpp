#include "base/renderer/camera_system.h"
#include "base/scene/component_manager.h"
#include "base/renderer/renderer_2d.h"

namespace Yogi {

    void CameraSystem::on_update(Timestep ts, Scene* scene)
    {
        YG_PROFILE_FUNCTION();

        scene->view_components({"TransformComponent", "CameraComponent"}, [ts](std::vector<void*> components){
            glm::vec3& translation = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "translation");
            glm::vec3& rotation = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "rotation");
            glm::vec3& scale = ComponentManager::field<glm::vec3>(components[0], "TransformComponent", "scale");
            glm::mat4 t = glm::translate(glm::mat4(1.0f), translation) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3(1, 0, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3(0, 1, 0)) *
                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3(0, 0, 1));
            glm::mat4 transform_inverse = glm::inverse(t);
            float& aspect_ratio = ComponentManager::field<float>(components[1], "CameraComponent", "aspect_ratio");
            float& zoom_level = ComponentManager::field<float>(components[1], "CameraComponent", "zoom_level");
            glm::mat4 projection_matrix = glm::ortho(-aspect_ratio * zoom_level, aspect_ratio * zoom_level, -zoom_level, zoom_level, -1.0f, 1.0f);
            Renderer2D::set_view_projection_matrix(projection_matrix * transform_inverse);
        });
    }

}