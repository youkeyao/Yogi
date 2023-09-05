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

    enum ColliderType
    {
        BOX = 0,
        SPHERE,
        CAPSULE
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
        bool cast_shadow = true;
    };

    struct CameraComponent
    {
        bool is_ortho = true;
        float fov = glm::radians(45.0f);
        float aspect_ratio = 1.0f;
        float zoom_level = 1.0f;
        Ref<RenderTexture> render_target = nullptr;
    };

    struct DirectionalLightComponent
    {
        Color color = glm::vec4(1.0f);
    };

    struct SpotLightComponent
    {
        float cutoff = glm::cos(glm::radians(12.5f));
        Color color = glm::vec4(1.0f);
    };

    struct PointLightComponent
    {
        float attenuation_parm = 1.0f;
        Color color = glm::vec4(1.0f);
    };

    struct RigidBodyComponent
    {
        bool is_static = false;
        glm::vec3 scale = glm::vec3(1.0f);
        ColliderType type = ColliderType::BOX;
    };

}