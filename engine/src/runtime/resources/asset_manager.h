#pragma once

#include "runtime/resources/material_manager.h"
#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/mesh_manager.h"
#include "runtime/resources/texture_manager.h"

namespace Yogi {

    class AssetManager
    {
    public:
        static void init()
        {
            TextureManager::init(YG_ASSET_DIR"textures");
            PipelineManager::init();
            MaterialManager::init();
            MeshManager::init();
        }
        static void shutdown()
        {
            TextureManager::clear();
            MaterialManager::clear();
            PipelineManager::clear();
        }
    };

}