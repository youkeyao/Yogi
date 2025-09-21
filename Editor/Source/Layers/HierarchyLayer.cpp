#include "Layers/HierarchyLayer.h"

#include "Reflect/ComponentManager.h"
#include "Reflect/SystemManager.h"

#include "Registry/AssetRegistry.h"

#include <imgui.h>

namespace Yogi
{

HierarchyLayer::HierarchyLayer(Handle<World>& world, Entity& selectedEntity) :
    Layer("Hierarchy Layer"),
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

        float                                             cursor_y = ImGui::GetCursorPosY();
        std::unordered_map<uint32_t, std::vector<Entity>> relations;
        std::vector<Entity>                               rootEntities;
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
            ComponentManager::EachComponentType([this](ComponentType& componentType) {
                std::string& componentName = componentType.Name;
                if (ImGui::MenuItem(componentName.c_str()))
                {
                    ComponentManager::AddComponent(m_selectedEntity, componentType.TypeHash);
                    ImGui::CloseCurrentPopup();
                }
            });
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
            SystemManager::EachSystemType([this](const std::string& systemName, uint32_t typeHash) {
                if (ImGui::MenuItem(systemName.c_str()))
                {
                    SystemManager::AddSystem(*m_world, typeHash);
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
    for (auto& child : relations[entity])
    {
        m_world->DeleteEntity(child);
    }
    m_world->DeleteEntity(entity);
}

void HierarchyLayer::DrawEntityNode(Entity& entity, std::unordered_map<uint32_t, std::vector<Entity>>& relations)
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

void HierarchyLayer::DrawComponents()
{
    m_selectedEntity.EachComponent([this](uint32_t typeHash, void* component) {
        ComponentType      componentType = ComponentManager::GetComponentType(typeHash);
        ImGuiTreeNodeFlags flags         = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        bool is_opened = ImGui::TreeNodeEx(component, flags, "%s", componentType.Name.c_str());
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete Component"))
            {
                ComponentManager::RemoveComponent(m_selectedEntity, componentType.TypeHash);
            }
            ImGui::EndPopup();
        }
        if (is_opened)
        {
            for (auto& field : componentType.Fields)
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
                    Transform& transform = *reinterpret_cast<Transform*>((uint8_t*)component + field.Offset);
                    ImGui::DragFloat3("Position", &transform.Position.x, 0.25f);
                    ImGui::DragFloat3("Rotation", &transform.Rotation.x, 0.25f);
                    ImGui::DragFloat3("Scale", &transform.Scale.x, 0.25f);
                }
                else if (field.TypeHash == GetTypeHash<Entity>())
                {
                    Entity& entity = *reinterpret_cast<Entity*>((uint8_t*)component + field.Offset);
                    if (ImGui::BeginCombo(field.Name.c_str(),
                                          entity ? entity.GetComponent<TagComponent>().Tag.c_str() : "None"))
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
                                if (CheckEntityParent(entity, m_selectedEntity))
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
                    std::string meshKey = AssetRegistry::GetKey<Mesh>(mesh);
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
                // else if (value.type_hash == typeid(Ref<Material>).hash_code())
                // {
                //     Ref<Material>& material = *(Ref<Material>*)((uint8_t*)component + value.offset);
                //     if (ImGui::BeginCombo(key.c_str(), (material->get_name()).c_str()))
                //     {
                //         MaterialManager::each_material([&](const Ref<Material>& each_material) {
                //             bool is_selected = material == each_material;
                //             if (ImGui::Selectable(each_material->get_name().c_str(), is_selected))
                //             {
                //                 material = each_material;
                //             }
                //         });
                //         ImGui::EndCombo();
                //     }
                // }
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
            ImGui::TreePop();
        }
    });
}

void HierarchyLayer::DrawSystems()
{
    uint32_t index = 0;
    m_world->EachSystem([this, &index](uint32_t systemHash) {
        std::string        systemName = SystemManager::GetSystemName(systemHash);
        ImGuiTreeNodeFlags flags      = ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_Framed |
            ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_AllowItemOverlap | ImGuiTreeNodeFlags_FramePadding;
        bool is_opened = ImGui::TreeNodeEx(systemName.data(), flags);
        if (ImGui::BeginPopupContextItem())
        {
            if (ImGui::MenuItem("Delete System"))
            {
                SystemManager::RemoveSystem(*m_world, systemHash);
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
                m_world->ChangeSystemOrder(old_index, index);
            }
            ImGui::EndDragDropTarget();
        }

        if (is_opened)
        {
            ImGui::TreePop();
        }

        index++;
    });
}

} // namespace Yogi
