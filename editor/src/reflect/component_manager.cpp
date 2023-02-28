#include "reflect/component_manager.h"

namespace Yogi {

    std::unordered_map<std::string, ComponentType> ComponentManager::m_component_types = {};

    void ComponentManager::init()
    {
        add_component_type<std::string>("TagComponent", {"tag"});
        add_component_type<glm::vec3, glm::vec3, glm::vec3>("TransformComponent", {"translation", "rotation", "scale"});
        add_component_type<glm::vec4, glm::vec2, glm::vec2>("SpriteRendererComponent", {"color", "texcoords_min", "texcoords_max"});
        add_component_type<bool, float, float>("CameraComponent", {"is_primary", "aspect_ratio", "zoom_level"});
    }

    uint32_t ComponentManager::get_component_size(std::string name)
    {
        YG_CORE_ASSERT(m_component_types.find(name) != m_component_types.end(), "Invalid component name!");
        return m_component_types[name].size;
    }

}