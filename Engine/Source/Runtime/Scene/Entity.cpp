#include "Scene/Entity.h"

namespace Yogi
{

Entity::Entity(entt::entity handle, View<entt::registry> registry) : m_entityHandle(handle), m_registry(registry) {}

} // namespace Yogi