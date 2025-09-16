#include "Layers/HierarchyLayer.h"

#include <imgui.h>

namespace Yogi
{

HierarchyLayer::HierarchyLayer(Handle<World>& world, Entity& selectedEntity) :
    m_world(world),
    m_selectedEntity(selectedEntity)
{}

HierarchyLayer::~HierarchyLayer() { m_allEntities.clear(); }

void HierarchyLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Hierarchy");
    if (m_world)
    {
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                Entity entity = m_world->CreateEntity();
                entity.AddComponent<TagComponent>("New Entity");
                entity.AddComponent<TransformComponent>();
            }
            ImGui::EndPopup();
        }

        float                                           cursor_y = ImGui::GetCursorPosY();
        std::unordered_map<uint32_t, std::list<Entity>> relations;
        std::list<Entity>                               rootEntities;
        m_allEntities.clear();
        m_world->EachEntity([&relations, &rootEntities, this](Entity entity) {
            m_allEntities.push_back(entity);
            Entity parent = entity.GetComponent<TransformComponent>().Parent;
            if (parent)
            {
                relations[parent].push_back(entity);
            }
            else
            {
                rootEntities.push_back(entity);
            }
        });
        for (auto& entity : rootEntities)
        {
            DrawEntityNode(entity, relations);
        }

        // blank space
        ImGui::SetCursorPosY(cursor_y);
        ImVec2 viewport_region_min = ImGui::GetWindowContentRegionMin();
        ImVec2 viewport_region_max = ImGui::GetWindowContentRegionMax();
        ImGui::InvisibleButton("blank",
                               { std::max(viewport_region_max.x - viewport_region_min.x, 1.0f),
                                 std::max(viewport_region_max.y - viewport_region_min.y, 1.0f) });
        if (ImGui::IsItemClicked())
        {
            m_selectedEntity = Entity::Null();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("hierarchy"))
            {
                Entity e                                    = *(Entity*)payload->Data;
                e.GetComponent<TransformComponent>().Parent = Entity::Null();
            }
            ImGui::EndDragDropTarget();
        }
    }
    ImGui::End();

    ImGui::Begin("Components");
    if (m_selectedEntity)
    {
        DrawComponents();
        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            ImGui::OpenPopup("AddComponent");
        if (ImGui::BeginPopup("AddComponent"))
        {
            // ComponentManager::each_component_type([this](std::string component_name) {
            //     if (ImGui::MenuItem(component_name.c_str()))
            //     {
            //         ComponentManager::add_component(m_selectedEntity, component_name);
            //         ImGui::CloseCurrentPopup();
            //     }
            // });
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::Begin("Systems");
    if (m_world)
    {
        DrawSystems();
        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            ImGui::OpenPopup("AddSystem");
        if (ImGui::BeginPopup("AddSystem"))
        {
            // SystemManager::each_system_type([this](std::string system_name) {
            //     if (ImGui::MenuItem(system_name.c_str()))
            //     {
            //         SystemManager::add_system(m_world, system_name);
            //         ImGui::CloseCurrentPopup();
            //     }
            // });
            ImGui::EndPopup();
        }
    }
    ImGui::End();
}

void HierarchyLayer::OnEvent(Event& event) {}

// --------------------------------------------------------------------------
bool HierarchyLayer::CheckEntityParent(Entity& entity, Entity& parent)
{
    Entity target = entity;
    while (target != parent && target != Entity::Null())
    {
        target = target.GetComponent<TransformComponent>().Parent;
    }
    return (target != Entity::Null());
}

void HierarchyLayer::DeleteEntityAndChildren(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations)
{
    for (auto& child : relations[entity])
    {
        m_world->DeleteEntity(child);
    }
    m_world->DeleteEntity(entity);
}

void HierarchyLayer::DrawEntityNode(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations)
{
    auto& tag = entity.GetComponent<TagComponent>().Tag;

    ImGuiTreeNodeFlags flags = (m_selectedEntity && m_selectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
    bool isOpened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
    if (ImGui::IsItemClicked())
    {
        m_selectedEntity = entity;
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
        {
            DeleteEntityAndChildren(entity, relations);
            if (m_selectedEntity == entity)
                m_selectedEntity = Entity::Null();
        }
        ImGui::EndPopup();
    }

    if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
    {
        ImGui::SetDragDropPayload("hierarchy", &entity, sizeof(Entity));
        ImGui::EndDragDropSource();
    }
    if (ImGui::BeginDragDropTarget())
    {
        if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("hierarchy"))
        {
            Entity drag = *(Entity*)payload->Data;
            if (!CheckEntityParent(entity, drag))
            {
                drag.GetComponent<TransformComponent>().Parent = entity;
            }
        }
        ImGui::EndDragDropTarget();
    }

    if (isOpened)
    {
        auto it = relations.find(entity);
        if (it != relations.end())
        {
            for (auto& child : it->second)
            {
                DrawEntityNode(child, relations);
            }
        }
        ImGui::TreePop();
    }
}

void HierarchyLayer::DrawComponents() {}

void HierarchyLayer::DrawSystems() {}

} // namespace Yogi
