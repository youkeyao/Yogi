#pragma once

#include <glm/glm.hpp>
#include "base/scene/entity.h"
#include "base/renderer/texture.h"

namespace Yogi {

    struct TagComponent
    {
        std::string tag = "";
    };

    struct TransformComponent
    {
        Entity parent;
        glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct SpriteRendererComponent
    {
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> texture = nullptr;
        glm::vec2 texcoords_min = { 0.0f, 0.0f };
        glm::vec2 texcoords_max = { 1.0f, 1.0f };
    };

    struct CameraComponent
    {
        bool is_ortho = true;
        bool is_primary = true;
        float fov = glm::radians(45.0f);
        float aspect_ratio = 1.0f;
        float zoom_level = 1.0f;
    };

}