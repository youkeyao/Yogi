#include "Layers/HierarchyLayer.h"

#include "Reflect/SystemManager.h"

#include "Registry/AssetRegistry.h"

#include <imgui.h>

namespace Yogi
{

HierarchyLayer::HierarchyLayer() : Layer("Hierarchy Layer")
{
    m_viewportLayer = Ref<ViewportLayer>::Cast(Application::GetInstance().GetLayer("Viewport Layer"));
}

HierarchyLayer::~HierarchyLayer() { m_allEntities.clear(); }

void HierarchyLayer::OnUpdate(Timestep ts)
{
    ImGui::Begin("Hierarchy");
    auto world          = m_viewportLayer->GetWorld();
    auto selectedEntity = m_viewportLayer->GetSelectedEntity();
    if (world)
    {
        if (ImGui::BeginPopupContextWindow())
        {
            if (ImGui::MenuItem("Create Empty Entity"))
            {
                Entity entity = world->CreateEntity();
                entity.AddComponent<TagComponent>("New Entity");
                entity.AddComponent<TransformComponent>();
            }
            ImGui::EndPopup();
        }

        float                                             cursorY = ImGui::GetCursorPosY();
        std::unordered_map<uint32_t, std::vector<Entity>> relations;
        std::vector<Entity>                               rootEntities;
        m_allEntities.clear();
        world->EachEntity([&relations, &rootEntities, this](Entity entity) {
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
        ImGui::SetCursorPosY(cursorY);
        ImVec2 viewportRegionMin = ImGui::GetWindowContentRegionMin();
        ImVec2 viewportRegionMax = ImGui::GetWindowContentRegionMax();
        ImGui::InvisibleButton("blank",
                               { std::max(viewportRegionMax.x - viewportRegionMin.x, 1.0f),
                                 std::max(viewportRegionMax.y - viewportRegionMin.y, 1.0f) });
        if (ImGui::IsItemClicked())
        {
            selectedEntity = Entity::Null();
            m_viewportLayer->SetSelectedEntity(selectedEntity);
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
    if (selectedEntity)
    {
        DrawComponents();
        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            ImGui::OpenPopup("AddComponent");
        if (ImGui::BeginPopup("AddComponent"))
        {
            ComponentManager::EachComponentType([&](ComponentType& componentType) {
                std::string& componentName = componentType.Name;
                if (ImGui::MenuItem(componentName.c_str()))
                {
                    ComponentManager::AddComponent(selectedEntity, componentType.TypeHash);
                    ImGui::CloseCurrentPopup();
                }
            });
            ImGui::EndPopup();
        }
    }
    ImGui::End();

    ImGui::Begin("Systems");
    if (world)
    {
        DrawSystems();
        if (ImGui::Button("+", { ImGui::GetContentRegionAvail().x, 0.0f }))
            ImGui::OpenPopup("AddSystem");
        if (ImGui::BeginPopup("AddSystem"))
        {
            SystemManager::EachSystemType([&](const std::string& systemName, uint32_t typeHash) {
                if (ImGui::MenuItem(systemName.c_str()))
                {
                    SystemManager::AddSystem(*world, typeHash);
                    ImGui::CloseCurrentPopup();
                }
            });
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

void HierarchyLayer::DeleteEntityAndChildren(Entity&                                            entity,
                                             std::unordered_map<uint32_t, std::vector<Entity>>& relations)
{
    auto world = m_viewportLayer->GetWorld();
    for (auto& child : relations[entity])
    {
        world->DeleteEntity(child);
    }
    world->DeleteEntity(entity);
}

void HierarchyLayer::DrawEntityNode(Entity& entity, std::unordered_map<uint32_t, std::vector<Entity>>& relations)
{
    auto& tag = entity.GetComponent<TagComponent>().Tag;

    auto               selectedEntity = m_viewportLayer->GetSelectedEntity();
    ImGuiTreeNodeFlags flags          = (selectedEntity && selectedEntity == entity ? ImGuiTreeNodeFlags_Selected : 0) |
        ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_FramePadding;
    bool isOpened = ImGui::TreeNodeEx((void*)(uint64_t)(uint32_t)entity, flags, "%s", tag.c_str());
    if (ImGui::IsItemClicked())
    {
        m_viewportLayer->SetSelectedEntity(entity);
    }

    if (ImGui::BeginPopupContextItem())
    {
        if (ImGui::MenuItem("Delete Entity"))
        {
            DeleteEntityAndChildren(entity, relations);
            if (selectedEntity == entity)
                m_viewportLayer->SetSelectedEntity(Entity::Null());
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

void HierarchyLayer::DrawComponents()
{
    auto selectedEntity = m_viewportLayer->GetSelectedEntity();
    selectedEntity.EachComponent([&](uint32_t typeHash, void* component) {
        ComponentType      componentType = ComponentManager::GetComponentType(typeHash);
        ImGuiTreeNodeFlags flags         = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        bool isOpened = ImGui::TreeNodeEx(component, flags, "%s", componentType.Name.c_str());
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
            {
                ComponentManager::RemoveComponent(selectedEntity, componentType.TypeHash);
            }
            ImGui::EndPopup();
        }
        if (isOpened)
        {
            for (auto& field : componentType.Fields)
            {
                DrawField(field, (uint8_t*)component);
            }
            ImGui::TreePop();
        }
    });
}

void HierarchyLayer::DrawSystems()
{
    uint32_t index = 0;
    auto     world = m_viewportLayer->GetWorld();
    world->EachSystem([&](uint32_t systemHash) {
        std::string        systemName = SystemManager::GetSystemName(systemHash);
        ImGuiTreeNodeFlags flags      = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        bool isOpened = ImGui::TreeNodeEx(systemName.data(), flags);
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete System"))
            {
                SystemManager::RemoveSystem(*world, systemHash);
            }
            ImGui::EndPopup();
        }

        if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None))
        {
            ImGui::SetDragDropPayload("system", &index, sizeof(Entity));
            ImGui::EndDragDropSource();
        }
        if (ImGui::BeginDragDropTarget())
        {
            if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("system"))
            {
                uint32_t old_index = *(uint32_t*)payload->Data;
                world->ChangeSystemOrder(old_index, index);
            }
            ImGui::EndDragDropTarget();
        }

        if (isOpened)
        {
            ImGui::TreePop();
        }

        index++;
    });
}

void HierarchyLayer::DrawField(Field& field, uint8_t* component)
{
    if (field.TypeHash == GetTypeHash<bool>())
    {
        bool* is = reinterpret_cast<bool*>((uint8_t*)component + field.Offset);
        ImGui::Checkbox(field.Name.c_str(), is);
    }
    else if (field.TypeHash == GetTypeHash<int>())
    {
        int* i = reinterpret_cast<int*>((uint8_t*)component + field.Offset);
        ImGui::InputInt(field.Name.c_str(), i);
    }
    else if (field.TypeHash == GetTypeHash<std::string>())
    {
        std::string& str = *reinterpret_cast<std::string*>((uint8_t*)component + field.Offset);
        char         buffer[256];
        memset(buffer, 0, sizeof(buffer));
        strcpy(buffer, str.c_str());
        if (ImGui::InputText(field.Name.c_str(), buffer, sizeof(buffer)))
        {
            str = std::string{ buffer };
        }
    }
    else if (field.TypeHash == GetTypeHash<float>())
    {
        float* f = reinterpret_cast<float*>((uint8_t*)component + field.Offset);
        ImGui::InputFloat(field.Name.c_str(), f, 0.25f);
    }
    else if (field.TypeHash == GetTypeHash<Vector2>())
    {
        Vector2& vec2 = *reinterpret_cast<Vector2*>((uint8_t*)component + field.Offset);
        ImGui::DragFloat2(field.Name.c_str(), &vec2.x, 0.25f);
    }
    else if (field.TypeHash == GetTypeHash<Vector3>())
    {
        Vector3& vec3 = *reinterpret_cast<Vector3*>((uint8_t*)component + field.Offset);
        ImGui::DragFloat3(field.Name.c_str(), &vec3.x, 0.25f);
    }
    else if (field.TypeHash == GetTypeHash<Vector4>())
    {
        Vector4& vec4 = *reinterpret_cast<Vector4*>((uint8_t*)component + field.Offset);
        ImGui::DragFloat4(field.Name.c_str(), &vec4.x, 0.25f);
    }
    else if (field.TypeHash == GetTypeHash<Color>())
    {
        Color& color = *reinterpret_cast<Color*>((uint8_t*)component + field.Offset);
        ImGui::ColorEdit4(field.Name.c_str(), &color.r, ImGuiColorEditFlags_Float);
    }
    else if (field.TypeHash == GetTypeHash<Transform>())
    {
        Transform& transform      = *reinterpret_cast<Transform*>((uint8_t*)component + field.Offset);
        auto       selectedEntity = m_viewportLayer->GetSelectedEntity();
        ImGui::DragFloat3("Position", &transform.Position.x, 0.25f);
        if (ImGui::DragFloat3("Rotation", &m_entitiesEulerAngles[selectedEntity].x, 0.25f))
            transform.Rotation = Quaternion(m_entitiesEulerAngles[selectedEntity]);
        ImGui::DragFloat3("Scale", &transform.Scale.x, 0.25f);
    }
    else if (field.TypeHash == GetTypeHash<Entity>())
    {
        Entity& entity         = *reinterpret_cast<Entity*>((uint8_t*)component + field.Offset);
        auto    selectedEntity = m_viewportLayer->GetSelectedEntity();
        if (ImGui::BeginCombo(field.Name.c_str(), entity ? entity.GetComponent<TagComponent>().Tag.c_str() : "None"))
        {
            // Set Null
            if (ImGui::Selectable("None", !entity))
            {
                entity = Entity::Null();
            }
            // Set other entities
            for (auto e : m_allEntities)
            {
                ImGui::PushID((uint32_t)e);
                const char* name       = e.GetComponent<TagComponent>().Tag.c_str();
                bool        isSelected = entity == e;
                if (ImGui::Selectable(name, isSelected))
                {
                    entity = e;
                    if (CheckEntityParent(entity, selectedEntity))
                    {
                        entity = Entity::Null();
                    }
                }
                ImGui::PopID();
            }
            ImGui::EndCombo();
        }
    }
    else if (field.TypeHash == GetTypeHash<Ref<Mesh>>())
    {
        Ref<Mesh>&  mesh    = *reinterpret_cast<Ref<Mesh>*>((uint8_t*)component + field.Offset);
        std::string meshKey = AssetManager::GetAssetKey(mesh);
        if (ImGui::BeginCombo(field.Name.c_str(), meshKey.c_str()))
        {
            for (auto& key : AssetRegistry::GetKeys<Mesh>())
            {
                if (ImGui::Selectable(key.c_str(), meshKey == key))
                {
                    mesh = AssetManager::GetAsset<Mesh>(key);
                }
            }
            ImGui::EndCombo();
        }
    }
    else if (field.TypeHash == GetTypeHash<Ref<Material>>())
    {
        Ref<Material>& material    = *(Ref<Material>*)((uint8_t*)component + field.Offset);
        std::string    materialKey = AssetManager::GetAssetKey(material);
        if (ImGui::BeginCombo(field.Name.c_str(), materialKey.c_str()))
        {
            for (auto& key : AssetRegistry::GetKeys<Material>())
            {
                if (ImGui::Selectable(key.c_str(), materialKey == key))
                {
                    material = AssetManager::GetAsset<Material>(key);
                }
            }
            ImGui::EndCombo();
        }
    }
    // else if (value.type_hash == typeid(Ref<RenderTexture>).hash_code())
    // {
    //     Ref<RenderTexture>& texture = *(Ref<RenderTexture>*)((uint8_t*)component + value.offset);
    //     if (ImGui::BeginCombo(key.c_str(), texture ? (texture->get_name()).c_str() : "None"))
    //     {
    //         TextureManager::each_render_texture([&](const Ref<RenderTexture>& each_texture) {
    //             bool is_selected = texture == each_texture;
    //             if (ImGui::Selectable(each_texture->get_name().c_str(), is_selected))
    //             {
    //                 texture = each_texture;
    //             }
    //         });
    //         if (ImGui::Selectable("None", texture == nullptr))
    //         {
    //             texture = nullptr;
    //         }
    //         ImGui::EndCombo();
    //     }
    // }
    // else if (value.type_hash == typeid(ColliderType).hash_code())
    // {
    //     ColliderType&            type       = *(ColliderType*)((uint8_t*)component + value.offset);
    //     std::vector<std::string> type_names = { "box", "sphere", "capsule" };
    //     if (ImGui::BeginCombo(key.c_str(), type_names[(int)type].c_str()))
    //     {
    //         for (int i = 0; i < type_names.size(); i++)
    //         {
    //             if (ImGui::Selectable(type_names[i].c_str(), type == i))
    //             {
    //                 type = (ColliderType)i;
    //             }
    //         }
    //         ImGui::EndCombo();
    //     }
    // }
}

} // namespace Yogi
