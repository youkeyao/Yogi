#include "runtime/scene/entity.h"

namespace Yogi {

Entity::Entity(entt::entity handle, entt::registry *registry) : m_entity_handle(handle), m_registry(registry) {}

}  // namespace Yogi