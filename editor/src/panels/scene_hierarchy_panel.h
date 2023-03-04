#pragma once

#include <engine.h>

namespace Yogi {

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(Ref<Scene> context);

        void on_imgui_render();

        Ref<Entity> get_selected_entity() const { return m_selected_entity; }
        void set_selected_entity(Ref<Entity> entity) { m_selected_entity = entity; }
    private:
        bool check_entity_parent(Entity entity, Entity parent);
        void delete_entity_and_children(Entity entity, std::unordered_map<uint32_t, std::list<Entity>>& relations);
        void draw_entity_node(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations);
        void draw_components();
        void draw_systems();
        void check_field(std::string component, std::string field, void* ptr);
    private:
        Ref<Scene> m_context;
        Ref<Entity> m_selected_entity;
        std::vector<Entity> m_all_entities;
    };

}