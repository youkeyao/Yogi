#include "Resources/AssetManager/AssetManager.h"

namespace Yogi
{

std::vector<Handle<IAssetSource>>                                                     AssetManager::s_sources;
std::unordered_map<std::type_index, std::unique_ptr<void, AssetManager::VoidDeleter>> AssetManager::s_serializers;
std::unordered_map<std::type_index, AssetManager::MapInfo>                            AssetManager::s_assetMaps;

} // namespace Yogi
