#pragma once

#include <engine.h>

namespace Yogi {

    class SceneHierarchyPanel
    {
    public:
        SceneHierarchyPanel(Ref<Scene> context);

        void on_imgui_render();

        Ref<Entity> GetSelectedEntity() const { return m_selected_entity; }
        void SetSelectedEntity(Entity entity);
    private:
        template<typename T>
        void DisplayAddComponentEntry(const std::string& entryName);
    
        void draw_entity_node(Ref<Entity> entity);
        void draw_components();
    private:
        Ref<Scene> m_context;
        Ref<Entity> m_selected_entity;
    };

}