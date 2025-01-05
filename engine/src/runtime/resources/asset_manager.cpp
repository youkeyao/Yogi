#include "runtime/resources/asset_manager.h"

namespace Yogi {

void AssetManager::init(const std::string &dir)
{
    TextureManager::init(dir);
    MaterialManager::init(dir);
    MeshManager::init(dir);
}

void AssetManager::init_project(const std::string &dir)
{
    clear();
    init(YG_ASSET_DIR);
    init(dir);
}

void AssetManager::clear()
{
    TextureManager::clear();
    MaterialManager::clear();
    MeshManager::clear();
}

}  // namespace Yogi