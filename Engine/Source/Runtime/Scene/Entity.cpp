#include "Scene/Entity.h"

namespace Yogi
{

Entity::Entity(entt::entity handle, const Ref<entt::registry>& registry) : m_entityHandle(handle), m_registry(registry) {}

} // namespace Yogi