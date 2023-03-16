#pragma once

#include <glm/glm.hpp>
#include "runtime/scene/entity.h"
#include "runtime/renderer/texture.h"

namespace Yogi {

    struct Transform
    {
        operator glm::mat4() const { return m_transform; }
        Transform& operator=(const glm::mat4& transform) { m_transform = transform; return *this; }
    private:
        glm::mat4 m_transform = glm::mat4(1.0f);
    };

    struct Color
    {
        operator glm::vec4() const { return m_color; }
        Color& operator=(const glm::vec4& color) { m_color = color; return *this; }
    private:
        glm::vec4 m_color = glm::vec4(1.0f);
    };

    struct TagComponent
    {
        std::string tag = "";
    };

    struct TransformComponent
    {
        Entity parent;
        Transform transform;
    };

    struct SpriteRendererComponent
    {
        Color color;
        Ref<Texture2D> texture = nullptr;
        glm::vec2 tex_min = { 0.0f, 0.0f };
        glm::vec2 tex_max = { 1.0f, 1.0f };
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