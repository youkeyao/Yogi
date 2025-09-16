#pragma once

#include <Yogi.h>

namespace Yogi
{

class HierarchyLayer : public Layer
{
public:
    HierarchyLayer(Handle<World>& world, Entity& selectedEntity);
    virtual ~HierarchyLayer() = default;

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    bool CheckEntityParent(Entity& entity, Entity& parent);
    void DeleteEntityAndChildren(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations);

    void DrawEntityNode(Entity& entity, std::unordered_map<uint32_t, std::list<Entity>>& relations);
    void DrawComponents();
    void DrawSystems();

private:
    Handle<World>&      m_world;
    Entity&             m_selectedEntity;
    std::vector<Entity> m_allEntities;
};

} // namespace Yogi
