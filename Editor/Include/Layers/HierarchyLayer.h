#pragma once

#include <Yogi.h>

#include "Layers/ViewportLayer.h"
#include "Reflect/ComponentManager.h"

namespace Yogi
{

class HierarchyLayer : public Layer
{
public:
    HierarchyLayer();
    virtual ~HierarchyLayer();

    void OnUpdate(Timestep ts) override;
    void OnEvent(Event& event) override;

private:
    bool CheckEntityParent(Entity& entity, Entity& parent);
    void DeleteEntityAndChildren(Entity& entity, std::unordered_map<uint32_t, std::vector<Entity>>& relations);

    void DrawEntityNode(Entity& entity, std::unordered_map<uint32_t, std::vector<Entity>>& relations);
    void DrawComponents();
    void DrawSystems();
    void DrawField(Field& field, uint8_t* component);

private:
    std::vector<Entity>       m_allEntities;
    std::map<Entity, Vector3> m_entitiesEulerAngles;
    Ref<ViewportLayer>        m_viewportLayer;
};

} // namespace Yogi
