#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "runtime/scene/entity.h"
#include "runtime/resources/material_manager.h"
#include "runtime/resources/mesh_manager.h"

namespace Yogi {

    // ---Component Element--------------------------------------------------
    struct Transform
    {
        Transform(const glm::mat4& transform) : m_transform(transform) {}
        operator glm::mat4() const { return m_transform; }
        Transform& operator=(const glm::mat4& transform) { m_transform = transform; return *this; }
    private:
        glm::mat4 m_transform = glm::mat4(1.0f);
    };

    struct Color
    {
        Color(const glm::vec4& color) : m_color(color) {}
        operator glm::vec4() const { return m_color; }
        Color& operator=(const glm::vec4& color) { m_color = color; return *this; }
    private:
        glm::vec4 m_color = glm::vec4(1.0f);
    };
    //---------------------------------------------------------------------------------------

    struct TagComponent
    {
        std::string tag = "";
    };

    struct TransformComponent
    {
        Entity parent = {};
        Transform transform = glm::mat4(1.0f);
    };

    struct MeshRendererComponent
    {
        Ref<Mesh> mesh = MeshManager::get_mesh("quad");
        Ref<Material> material = MaterialManager::get_material("");
    };

    struct CameraComponent
    {
        bool is_ortho = true;
        float fov = glm::radians(45.0f);
        float aspect_ratio = 1.0f;
        float zoom_level = 1.0f;
        Ref<Texture> render_target = nullptr;
    };

}