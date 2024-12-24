#include "runtime/resources/asset_manager.h"

namespace Yogi {

void AssetManager::init_project(const std::string &dir)
{
    shutdown();
    init(YG_ASSET_DIR);
    init(dir);
}

}  // namespace Yogi