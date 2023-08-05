#include "panels/scene_hierarchy_panel.h"
#include "reflect/component_manager.h"
#include "reflect/system_manager.h"
#include <imgui.h>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/quaternion.hpp>

namespace Yogi {

    SceneHierarchyPanel::SceneHierarchyPanel(Ref<Scene> scene) : m_scene(scene)
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
            m_scene->delete_entity(child);
        }
        m_scene->delete_entity(entity);
    }

    void SceneHierarchyPanel::on_imgui_render()
    {
        ImGui::Begin("Hierarchy");
        if (m_scene) {
            if (ImGui::BeginPopupContextWindow()) {
                if (ImGui::MenuItem("Create Empty Entity")) {
                    Entity entity = m_scene->create_entity();
                    entity.add_component<TagComponent>("New Entity");
                    entity.add_component<TransformComponent>();
                }
                ImGui::EndPopup();
            }

            float cursor_y = ImGui::GetCursorPosY();
            std::unordered_map<uint32_t, std::list<Entity>> relations;
            m_all_entities.clear();
            m_all_entities.push_back(Entity{});
            m_scene->each_entity([&relations, this](Entity entity){
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
                m_selected_entity = Entity{};
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
                        ComponentManager::add_component(m_selected_entity, component_name);
                        ImGui::CloseCurrentPopup();
                    }
                });
                ImGui::EndPopup();
            }
        }
        ImGui::End();

        ImGui::Begin("Systems");
        if (m_scene) {
            draw_systems();
            if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
                ImGui::OpenPopup("AddSystem");
            if (ImGui::BeginPopup("AddSystem")) {
                SystemManager::each_system_type([this](std::string system_name){
                    if (ImGui::MenuItem(system_name.c_str())) {
                        SystemManager::add_system(m_scene, system_name);
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
        
        ImGuiTreeNodeFlags flags = (m_selected_entity && m_selected_entity == entity ? ImGuiTreeNodeFlags_Selected : 0) |
            ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
        bool is_opened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
        if (ImGui::IsItemClicked()) {
            m_selected_entity = entity;
        }

        if (ImGui::BeginPopupContextItem()) {
            if (ImGui::MenuItem("Delete Entity")) {
                delete_entity_and_children(entity, relations);
                if (m_selected_entity == entity) m_selected_entity = Entity{};
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
        m_selected_entity.each_component([this](std::string_view name, void* component){
            std::string component_name{name};
            ComponentType type = ComponentManager::get_component_type(component_name);
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            bool is_opened = ImGui::TreeNodeEx(component, flags, "%s", component_name.c_str());
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete Component")) {
                    ComponentManager::remove_component(m_selected_entity, component_name);
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
                        ImGui::DragFloat4(key.c_str(), glm::value_ptr(vec4));
                    }
                    else if (value.type_hash == typeid(Color).hash_code()) {
                        glm::vec4& color = *(glm::vec4*)((uint8_t*)component + value.offset);
                        ImGui::ColorEdit4(key.c_str(), glm::value_ptr(color));
                    }
                    else if (value.type_hash == typeid(Transform).hash_code()) {
                        glm::mat4& transform = *(glm::mat4*)((uint8_t*)component + value.offset);
                        glm::vec3 translation = {transform[3][0], transform[3][1], transform[3][2]};
                        glm::vec3 scale;
                        scale.x = glm::length(glm::vec3{transform[0][0], transform[0][1], transform[0][2]});
                        scale.y = glm::length(glm::vec3{transform[1][0], transform[1][1], transform[1][2]});
                        scale.z = glm::length(glm::vec3{transform[2][0], transform[2][1], transform[2][2]});
                        glm::vec3 rotation;
                        if (scale.z == 0)
                            rotation.y = asin(transform[2][0] / glm::epsilon<float>());
                        else
                            rotation.y = asin(transform[2][0] / scale.z);
                        if (glm::abs(cos(rotation.y)) >= 0.001) {
                            rotation.x = -atan2(transform[2][1], transform[2][2]);
                            rotation.z = -atan2(transform[1][0], transform[0][0]);
                        } else {
                            rotation.x = atan2(transform[1][2], transform[1][1]);
                            rotation.z = 0;
                        }
                        rotation *= 180.0f / glm::pi<float>();
                        if (ImGui::DragFloat3("position", glm::value_ptr(translation), 0.25f)) {
                            transform = glm::translate(glm::mat4(1.0f), translation) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3{1.0f, 0, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3{0, 1.0f, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3{0, 0, 1.0f}) *
                                glm::scale(glm::mat4(1.0f), scale);
                        }
                        if (ImGui::DragFloat3("rotation", glm::value_ptr(rotation), 0.25f)) {
                            transform = glm::translate(glm::mat4(1.0f), translation) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3{1.0f, 0, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3{0, 1.0f, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3{0, 0, 1.0f}) *
                                glm::scale(glm::mat4(1.0f), scale);
                        }
                        if (ImGui::DragFloat3("scale", glm::value_ptr(scale), 0.25f)) {
                            transform = glm::translate(glm::mat4(1.0f), translation) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.x), glm::vec3{1.0f, 0, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.y), glm::vec3{0, 1.0f, 0}) *
                                glm::rotate(glm::mat4(1.0f), glm::radians(rotation.z), glm::vec3{0, 0, 1.0f}) *
                                glm::scale(glm::mat4(1.0f), scale);
                        }
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
                            }
                            ImGui::EndCombo();
                        }
                    }
                    else if (value.type_hash == typeid(Ref<Mesh>).hash_code()) {
                        Ref<Mesh>& mesh = *(Ref<Mesh>*)((uint8_t*)component + value.offset);
                        if (ImGui::BeginCombo(key.c_str(), (mesh->name).c_str())) {
                            MeshManager::each_mesh_name([&](std::string mesh_name){
                                Ref<Mesh> each_mesh = MeshManager::get_mesh(mesh_name);
                                bool is_selected = mesh == each_mesh;
                                if (ImGui::Selectable(mesh_name.c_str(), is_selected)) {
                                    mesh = each_mesh;
                                }
                            });
                            ImGui::EndCombo();
                        }
                    }
                    else if (value.type_hash == typeid(Ref<Material>).hash_code()) {
                        Ref<Material>& material = *(Ref<Material>*)((uint8_t*)component + value.offset);
                        if (ImGui::BeginCombo(key.c_str(), (material->get_name()).c_str())) {
                            MaterialManager::each_material([&](const Ref<Material>& each_material){
                                bool is_selected = material == each_material;
                                if (ImGui::Selectable(each_material->get_name().c_str(), is_selected)) {
                                    material = each_material;
                                }
                            });
                            ImGui::EndCombo();
                        }
                    }
                    else if (value.type_hash == typeid(Ref<RenderTexture>).hash_code()) {
                        Ref<RenderTexture>& texture = *(Ref<RenderTexture>*)((uint8_t*)component + value.offset);
                        if (ImGui::BeginCombo(key.c_str(), texture ? (texture->get_name()).c_str() : "None")) {
                            TextureManager::each_render_texture([&](const Ref<RenderTexture>& each_texture){
                                bool is_selected = texture == each_texture;
                                if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected)) {
                                    texture = each_texture;
                                }
                            });
                            ImGui::EndCombo();
                        }
                    }
                }
                ImGui::TreePop();
            }
        });
    }

    void SceneHierarchyPanel::draw_systems()
    {
        uint32_t index = 0;
        m_scene->each_system([this, &index](std::string system_name, int32_t update_pos, int32_t event_pos){
            ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
                ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
            bool is_opened = ImGui::TreeNodeEx(system_name.c_str(), flags);
            if (ImGui::BeginPopupContextItem()) {
                if (ImGui::MenuItem("Delete System")) {
                    SystemManager::remove_system(m_scene, system_name);
                }
                ImGui::EndPopup();
            }

            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("system", &index, sizeof(Entity));
                ImGui::EndDragDropSource();
            }
            if (ImGui::BeginDragDropTarget()) {
                if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("system")) {
                    uint32_t old_index = *(uint32_t*)payload->Data;
                    m_scene->change_system_order(old_index, index);
                }
                ImGui::EndDragDropTarget();
            }

            if (is_opened) {
                if (update_pos >= 0) {
                    ImGui::Text("%s::on_update", system_name.c_str());
                }
                if (event_pos >= 0) {
                    ImGui::Text("%s::on_event", system_name.c_str());
                }
                ImGui::TreePop();
            }

            index ++;
        });
    }

    void SceneHierarchyPanel::check_field(std::string component_name, std::string field, void* ptr)
    {
        ComponentType type = ComponentManager::get_component_type(component_name);
        if (component_name == "Yogi::TransformComponent" && field == "parent") {
            std::size_t offset = type.m_fields[field].offset;
            Entity& entity = *(Entity*)((uint8_t*)ptr + offset);
            if (check_entity_parent(entity, m_selected_entity) && check_entity_parent(m_selected_entity, entity)) {
                entity = Entity{};
            }
        }
    }

}