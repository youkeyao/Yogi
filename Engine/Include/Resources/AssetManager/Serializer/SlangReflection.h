#pragma once

// Reflection helpers for Slang-driven material schemas.
//
// Phase 4 of the GLSL->Slang migration: instead of hand-coded
// `schema.AddField(...)` calls in C++, we ask Slang to reflect the layout
// of a concrete IMaterial impl after specialization, and translate that
// reflection into a MaterialSchema (offsets + sizes + editor hints).
//
// The single entry point is `BuildMaterialSchema`, called by
// SlangShaderCompiler::CompileSpecializedFragment once it has a linked
// program in hand. Failures log + return false; the caller should fall
// back to a hand-coded schema (Phase 4.4 keeps a kStandard fallback for
// exactly this reason).

#include "Renderer/MaterialSchema.h"

#include <filesystem>
#include <string>

namespace slang
{
struct IComponentType;
struct IGlobalSession;
} // namespace slang

namespace Yogi
{

bool BuildMaterialSchemaFromReflection(slang::IGlobalSession* globalSession,
                                       slang::IComponentType* linkedProgram,
                                       const std::string&     materialTypeName,
                                       MaterialSchema&        outSchema);

} // namespace Yogi
