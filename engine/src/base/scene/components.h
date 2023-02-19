#pragma once

#include <glm/glm.hpp>
#include "base/renderer/texture.h"

namespace Yogi {

    struct TransformComponent
    {
        glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
        glm::mat4 transform_inverse = glm::mat4(1.0f);
    };

    struct SpriteRendererComponent
    {
        glm::vec4 color = { 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> texture = nullptr;
        std::pair<glm::vec2, glm::vec2> texcoords = {{ 1.0f, 1.0f }, { 1.0f, 1.0f }};
    };

    struct CameraComponent
    {
        bool is_primary = true;
        float aspect_ratio = 1.0f;
        float zoom_level = 1.0f;
        glm::mat4 projection_matrix = glm::mat4(1.0f);
    };

}