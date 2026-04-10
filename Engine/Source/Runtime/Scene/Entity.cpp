#include "Scene/Entity.h"

namespace Yogi
{

Entity::Entity(entt::entity handle, const WRef<entt::registry>& registry) : m_entityHandle(handle), m_registry(registry)
{}

} // namespace Yogi