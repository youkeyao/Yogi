#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

std::unordered_map<std::type_index, ResourceManager::Any> ResourceManager::s_resourceMaps;

} // namespace Yogi
