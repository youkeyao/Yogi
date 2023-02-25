#include "base/scene/entity.h"

namespace Yogi {

    Entity::Entity(entt::entity handle, Ref<entt::registry> registry)
        : m_entity_handle(handle), m_registry(registry)
    {
    }
    
}