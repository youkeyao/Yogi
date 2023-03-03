#include "panels/scene_hierarchy_panel.h"
#include "reflect/component_manager.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>

namespace Yogi {

    SceneHierarchyPanel::SceneHierarchyPanel(Ref<Scene> context) : m_context(context)
    {}

    bool SceneHierarchyPanel::check_entity_parent(Entity entity, Entity parent)
    {
        Entity target = entity;
        while (target != parent && target != Entity{}) {
            target = target.get_component<TransformComponent>().parent;
        }
        return (target != Entity{});
    }

    void SceneHierarchyPanel::delete_entity_and_children(Entity entity, std::unordered_map<uint32_t, std::list<Entity>>& relations)
    {
        for (auto& child : relations[entity]) {
            m_context->delete_entity(child);
        }
        m_context->delete_entity(entity);
    }

    void SceneHierarchyPanel::on_imgui_render()
    {
        ImGui::Begin("Scene Hierarchy");

        if (m_context) {
            if (ImGui::BeginPopupContextWindow()) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    Entity entity = m_context->create_entity();
                    entity.add_component<TagComponent>("New Entity");
                    entity.add_component<TransformComponent>();
                }
                ImGui::EndPopup();
            }

            float cursor_y = ImGui::GetCursorPosY();
            std::unordered_map<uint32_t, std::list<Entity>> relations;
            m_all_entities.clear();
            m_all_entities.push_back(Entity{});
            m_context->each_entity([&relations, this](Entity entity){
                m_all_entities.push_back(entity);
                Entity parent = entity.get_component<TransformComponent>().parent;
                if (parent) {
                    relations[parent].push_back(entity);
                }
                else {
                    relations[entity].push_front(entity);
                }
            });
            for (auto& [i, list] : relations) {
                Entity front = list.front();
                if (i == (uint32_t)front) {
                    list.pop_front();
                    draw_entity_node(front, relations);
                }
            }

            // blank space
            ImGui::SetCursorPosY(cursor_y);
            ImVec2 viewport_region_min = ImGui::GetWindowContentRegionMin();
            ImVec2 viewport_region_max = ImGui::GetWindowContentRegionMax();
            ImGui::InvisibleButton("blank", { std::max(viewport_region_max.x - viewport_region_min.x, 1.0f), std::max(viewport_region_max.y - viewport_region_min.y, 1.0f) });
            if (ImGui::IsItemClicked()) {
                m_selected_entity.reset();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("hierarchy")) {
                    Entity e = *(Entity*)payload->Data;
                    e.get_component<TransformComponent>().parent = Entity{};
                }
                ImGui::EndDragDropTarget();
            }
        }
        ImGui::End();

        ImGui::Begin("Components");
        if (m_selected_entity) {
            draw_components();
            if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
                ImGui::OpenPopup("AddComponent");
            if (ImGui::BeginPopup("AddComponent")) {
                ComponentManager::each_component_type([this](std::string component_name){
                    if (ImGui::MenuItem(component_name.c_str())) {
                        ComponentManager::add_component(*m_selected_entity, component_name);
                        ImGui::CloseCurrentPopup();
                    }
                });
                ImGui::EndPopup();
            }
        }

        ImGui::End();
    }

    void SceneHierarchyPanel::draw_entity_node(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations)
    {
        auto& tag = entity.get_component<TagComponent>().tag;
        
        ImGuiTreeNodeFlags flags = (m_selected_entity && (*m_selected_entity) == entity ? ImGuiTreeNodeFlags_Selected : 0) |
            ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        bool is_opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
        if (ImGui::IsItemClicked()) {
            m_selected_entity = CreateRef<Entity>(entity);
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                delete_entity_and_children(entity, relations);
                if (*m_selected_entity == entity) m_selected_entity.reset();
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
            ImGui::SetDragDropPayload("hierarchy", &entity, sizeof(Entity));
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget()) {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("hierarchy")) {
                Entity drag = *(Entity*)payload->Data;
                if (!check_entity_parent(entity, drag)) {
                    drag.get_component<TransformComponent>().parent = entity;
                }
            }
            ImGui::EndDragDropTarget();
        }

        if (is_opened) {
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
            for (auto& child : relations[entity]) {
                draw_entity_node(child, relations);
            }
            ImGui::TreePop();
        }
    }

    void SceneHierarchyPanel::draw_components()
    {
        m_selected_entity->each_component([this](std::string_view name, void* component){
            std::string component_name{name};
            ComponentType type = ComponentManager::get_component_type(component_name);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            bool is_opened = ImGui::TreeNodeEx(component, flags, "%s", component_name.c_str());
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete Component")) {
                    ComponentManager::remove_component(*m_selected_entity, component_name);
                }
                ImGui::EndPopup();
            }
            if (is_opened) {
                for (auto [key, value] : type.m_fields) {
                    if (value.type_hash == typeid(bool).hash_code()) {
                        bool* is = (bool*)((uint8_t*)component + value.offset);
                        ImGui::Checkbox(key.c_str(), is);
                    }
                    else if (value.type_hash == typeid(std::string).hash_code()) {
                        std::string& str = *(std::string*)((uint8_t*)component + value.offset);
                        char buffer[256];
                        memset(buffer, 0, sizeof(buffer));
                        strcpy(buffer, str.c_str());
                        if (ImGui::InputText(key.c_str(), buffer, sizeof(buffer))) {
                            str = std::string{buffer};
                        }
                    }
                    else if (value.type_hash == typeid(float).hash_code()) {
                        float* f = (float*)((uint8_t*)component + value.offset);
                        ImGui::InputFloat(key.c_str(), f, 0.25f);
                    }
                    else if (value.type_hash == typeid(glm::vec2).hash_code()) {
                        glm::vec2& vec2 = *(glm::vec2*)((uint8_t*)component + value.offset);
                        ImGui::DragFloat2(key.c_str(), glm::value_ptr(vec2), 0.25f);
                    }
                    else if (value.type_hash == typeid(glm::vec3).hash_code()) {
                        glm::vec3& vec3 = *(glm::vec3*)((uint8_t*)component + value.offset);
                        ImGui::DragFloat3(key.c_str(), glm::value_ptr(vec3), 0.25f);
                    }
                    else if (value.type_hash == typeid(glm::vec4).hash_code()) {
                        glm::vec4& vec4 = *(glm::vec4*)((uint8_t*)component + value.offset);
                        ImGui::ColorEdit4(key.c_str(), glm::value_ptr(vec4));
                    }
                    else if (value.type_hash == typeid(Entity).hash_code()) {
                        Entity& entity = *(Entity*)((uint8_t*)component + value.offset);
                        if (ImGui::BeginCombo(key.c_str(), entity ? entity.get_component<TagComponent>().tag.c_str() : "None")) {
                            for (auto e : m_all_entities) {
                                const char* name = e ? e.get_component<TagComponent>().tag.c_str() : "None";
                                bool is_selected = entity == e;
                                if (ImGui::Selectable(name, is_selected)) {
                                    entity = e;
                                    check_field(component_name, key, component);
                                }
                                if (is_selected) ImGui::SetItemDefaultFocus();
                            }
                            ImGui::EndCombo();
                        }
                    }
                }
                ImGui::TreePop();
            }
        });
    }

    void SceneHierarchyPanel::check_field(std::string component_name, std::string field, void* ptr)
    {
        ComponentType type = ComponentManager::get_component_type(component_name);
        if (component_name == "Yogi::TransformComponent" && field == "parent") {
            std::size_t offset = type.m_fields[field].offset;
            Entity& entity = *(Entity*)((uint8_t*)ptr + offset);
            if (check_entity_parent(entity, *m_selected_entity) && check_entity_parent(*m_selected_entity, entity)) {
                entity = Entity{};
            }
        }
    }

}