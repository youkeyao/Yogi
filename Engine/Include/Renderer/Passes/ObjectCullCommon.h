#pragma once

#include "Renderer/ShaderData.h"

namespace Yogi::ObjectCull
{

static constexpr uint64_t MAX_MESHLET_VIS_COUNT          = 32ull * 1024ull * 1024ull;
static constexpr uint64_t INDIRECT_COMMAND_BUFFER_SIZE   = MAX_MESH_DRAWS * sizeof(uint32_t) * 3;
static constexpr uint64_t VISIBLE_DRAW_INDEX_BUFFER_SIZE = MAX_MESH_DRAWS * sizeof(uint32_t);
static constexpr uint64_t INDIRECT_COUNT_BUFFER_SIZE     = 64ull;
static constexpr uint64_t OBJECT_VIS_BITFIELD_SIZE       = ((MAX_MESH_DRAWS + 31ull) / 32ull) * sizeof(uint32_t);
static constexpr uint64_t MESHLET_VIS_BITFIELD_SIZE      = ((MAX_MESHLET_VIS_COUNT + 31ull) / 32ull) * sizeof(uint32_t);

static constexpr const char* SHADER_EARLY = "EngineAssets/Shaders/Passes/ObjectCull.cs.slang";
static constexpr const char* SHADER_LATE  = "EngineAssets/Shaders/Passes/ObjectCull.cs.slang::LATE=1";

} // namespace Yogi::ObjectCull
