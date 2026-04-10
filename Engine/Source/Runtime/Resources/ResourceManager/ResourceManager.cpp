#include "Resources/ResourceManager/ResourceManager.h"

namespace Yogi
{

std::unordered_map<std::type_index, ResourceManager::Entry> ResourceManager::s_resourceMaps;
std::unordered_map<std::type_index, ResourceManager::Entry> ResourceManager::s_resourceLists;

} // namespace Yogi
