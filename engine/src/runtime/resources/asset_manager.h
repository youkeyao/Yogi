#pragma once

#include "runtime/resources/material_manager.h"
#include "runtime/resources/mesh_manager.h"
#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/texture_manager.h"

namespace Yogi {

class AssetManager
{
public:
    static void init(const std::string &dir)
    {
        TextureManager::init(dir);
        MaterialManager::init(dir);
        MeshManager::init(dir);
    }
    static void init_project(const std::string &dir);
    static void shutdown()
    {
        TextureManager::clear();
        MaterialManager::clear();
        MeshManager::clear();
    }
};

}  // namespace Yogi