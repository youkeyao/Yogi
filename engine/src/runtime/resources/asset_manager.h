#pragma once

#include "runtime/resources/material_manager.h"
#include "runtime/resources/mesh_manager.h"
#include "runtime/resources/pipeline_manager.h"
#include "runtime/resources/texture_manager.h"

namespace Yogi {

class AssetManager
{
public:
    static void init(const std::string &dir);
    static void init_project(const std::string &dir);
    static void clear();
};

}  // namespace Yogi