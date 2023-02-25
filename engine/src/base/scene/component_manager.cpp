#include "component_manager.h"

namespace Yogi {

    // std::unordered_map<std::string, ComponentType> ComponentManager::m_component_types = {};

    // void ComponentManager::init()
    // {
    //     ComponentType tag_component;
    //     tag_component.fields_name["tag"] = 0;
    //     m_component_types["TagComponent"] = tag_component;

    //     ComponentType transform_component;
    //     transform_component.fields_name["translation"] = 0;
    //     transform_component.fields_name["rotation"] = 1;
    //     transform_component.fields_name["scale"] = 2;
    //     m_component_types["TransformComponent"] = transform_component;

    //     ComponentType sprite_renderer_component;
    //     sprite_renderer_component.fields_name["color"] = 0;
    //     sprite_renderer_component.fields_name["texcoords_min"] = 1;
    //     sprite_renderer_component.fields_name["texcoords_max"] = 2;
    //     m_component_types["SpriteRendererComponent"] = sprite_renderer_component;

    //     ComponentType camera_component;
    //     camera_component.fields_name["is_primary"] = 0;
    //     camera_component.fields_name["aspect_ratio"] = 1;
    //     camera_component.fields_name["zoom_level"] = 2;
    //     m_component_types["CameraComponent"] = camera_component;
    // }

    // void ComponentManager::add_component_type(std::string name, ComponentType type)
    // {
    //     m_component_types[name] = type;
    // }

    // uint32_t ComponentManager::get_component_size(std::string type)
    // {
    //     return m_component_types[type].size;
    // }

}