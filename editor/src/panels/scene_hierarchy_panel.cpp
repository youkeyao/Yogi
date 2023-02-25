#include "panels/scene_hierarchy_panel.h"
#include <imgui.h>

namespace Yogi {

    SceneHierarchyPanel::SceneHierarchyPanel(Ref<Scene> context) : m_context(context)
    {}

    void SceneHierarchyPanel::on_imgui_render()
    {
        ImGui::Begin("Scene Hierarchy");

        if (m_context)
        {
            m_context->each_entity([this](Ref<Entity> entity){
                draw_entity_node(entity);
            });

            // if (ImGui::IsMouseDown(0) && ImGui::IsWindowHovered())
            //     m_SelectionContext = {};

            // // Right-click on blank space
            // if (ImGui::BeginPopupContextWindow(0, 1, false))
            // {
            //     if (ImGui::MenuItem("Create Empty Entity"))
            //         m_Context->CreateEntity("Empty Entity");

            //     ImGui::EndPopup();
            // }

        }
        ImGui::End();

        ImGui::Begin("Properties");
		if (m_selected_entity) {
			draw_components();
		}

		ImGui::End();
    }

    void SceneHierarchyPanel::draw_entity_node(Ref<Entity> entity)
    {
        // auto& tag = entity->get_component<TagComponent>().tag;
        
        // ImGuiTreeNodeFlags flags = (m_selected_entity && (*m_selected_entity) == (*entity) ? ImGuiTreeNodeFlags_Selected : 0)
        //     | ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        // bool opened = ImGui::TreeNodeEx(tag.c_str(), flags);
        // if (ImGui::IsItemClicked()) {
        //     m_selected_entity = entity;
        // }

        // bool entityDeleted = false;
        // if (ImGui::BeginPopupContextItem()) {
        //     if (ImGui::MenuItem("Delete Entity"))
        //         entityDeleted = true;
        //     ImGui::EndPopup();
        // }

        // if (opened) {
        //     ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        //     bool opened = ImGui::TreeNodeEx(tag.c_str(), flags);
        //     if (opened)
        //         ImGui::TreePop();
        //     ImGui::TreePop();
        // }

        // if (entityDeleted) {
        //     m_context->delete_entity(entity);
        //     if (m_selected_entity == entity)
        //         m_selected_entity = nullptr;
        // }
    }

    void SceneHierarchyPanel::draw_components()
    {
        // Field Int;
        // Int.name = "int";
        // Int.size = sizeof(int);
        // Int.offset = 0;
        // ComponentType type;
        // type.fields.emplace("tag", Int);
        // type.size = Int.size;
        // ComponentManager::add_component_type("TagComponent", type);
        // Ref<uint8_t[]> component = ComponentManager::create_component("TagComponent");
        // ComponentManager::set_field("TagComponent", component, "tag", 100);
        // int tmp = ComponentManager::get_field<int>("TagComponent", component, "tag");
        // YG_CORE_INFO("{0}", tmp);
        // m_selected_entity->each_component([](){});
        // YG_CORE_INFO(std::tuple_size<decltype(m_selected_entity->get_all_components())>::value);
        // std::apply([](auto&&... args){
        //     ((std::cout << args << '\n'), ...);
        // }, m_selected_entity->get_all_components());
    }

}