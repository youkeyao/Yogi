#pragma once

#include <glm/glm.hpp>
#include "base/renderer/texture.h"

namespace Yogi {

    struct TransformComponent
    {
        glm::vec3 translation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 rotation = { 0.0f, 0.0f, 0.0f };
        glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
    };

    struct SpriteRendererComponent
    {
        glm::vec4 color{ 1.0f, 1.0f, 1.0f, 1.0f };
        Ref<Texture2D> texture;
        float tiling_factor = 1.0f;
    };

}