#include "Resources/AssetManager/Serializer/SlangShaderCompiler.h"
#include "Resources/AssetManager/Serializer/SlangReflection.h"

#include <slang.h>
#include <slang-com-ptr.h>

namespace Yogi
{

// Map Yogi's ShaderStage to Slang's SlangStage. The two enums are unrelated;
// we centralise the table so future stage additions stay in one place.
static SlangStage YgShaderStage2SlangStage(ShaderStage stage)
{
    switch (stage)
    {
        case ShaderStage::Vertex:
            return SLANG_STAGE_VERTEX;
        case ShaderStage::Geometry:
            return SLANG_STAGE_GEOMETRY;
        case ShaderStage::Fragment:
            return SLANG_STAGE_FRAGMENT;
        case ShaderStage::Compute:
            return SLANG_STAGE_COMPUTE;
        case ShaderStage::Task:
            return SLANG_STAGE_AMPLIFICATION;
        case ShaderStage::Mesh:
            return SLANG_STAGE_MESH;
    }
    return SLANG_STAGE_NONE;
}

struct SlangShaderCompiler::Impl
{
    Slang::ComPtr<slang::IGlobalSession> globalSession;
    std::mutex                           sessionMutex;
};

SlangShaderCompiler::SlangShaderCompiler() : m_impl(new Impl())
{
    SlangResult result = slang::createGlobalSession(m_impl->globalSession.writeRef());
    if (SLANG_FAILED(result))
    {
        YG_CORE_ERROR("Slang: failed to create global session (result {0})", (int)result);
        return;
    }
}

SlangShaderCompiler::~SlangShaderCompiler()
{
    delete m_impl;
    m_impl = nullptr;
}

std::vector<uint8_t> SlangShaderCompiler::Compile(const std::string&                                      slangSource,
                                                  const std::filesystem::path&                            sourcePath,
                                                  const std::string&                                      entryPoint,
                                                  ShaderStage                                             stage,
                                                  const std::vector<std::pair<std::string, std::string>>& macros)
{
    auto& self = Get();
    if (!self.m_impl || !self.m_impl->globalSession)
    {
        YG_CORE_ERROR("Slang: global session not available");
        return {};
    }

    // Slang sessions are cheap; spin up a fresh one per compile so per-shader
    // macro defines don't leak across compiles.
    std::lock_guard<std::mutex> lock(self.m_impl->sessionMutex);

    // ---- Target: Vulkan SPIR-V 1.5, scalar block layout, BDA enabled. ----
    slang::TargetDesc targetDesc{};
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = self.m_impl->globalSession->findProfile("spirv_1_5");
    targetDesc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
    // forceGLSLScalarBufferLayout: gives us std430-style packing with no
    // surprise vec3 padding - matches what BufferRefs.glsl currently relies
    // on. Toggle if/when shaders move off scalar layout.
    // targetDesc.forceGLSLScalarBufferLayout = true;

    // ---- Search paths: source's own directory + EngineAssets/Shaders root + "EngineInclude".
    // Mirrors FileIncluder's search order in ShaderSerializer.cpp.
    //
    // 1. shaderDir = source's parent (e.g. EngineAssets/Shaders/Passes/)
    // 2. shaderRoot = shaderDir.parent_path() (e.g. EngineAssets/Shaders/)
    //    Needed so `import Shared.ShaderData` resolves to Shared/ShaderData.slang
    //    regardless of which subdirectory the source lives in.
    // 3. EngineInclude = next to the executable (copied by PRE_BUILD).
    std::filesystem::path shaderDir     = sourcePath.parent_path();
    std::filesystem::path shaderRoot    = shaderDir.parent_path(); // EngineAssets/Shaders/
    std::string           shaderDirStr  = shaderDir.string();
    std::string           shaderRootStr = shaderRoot.string();
    std::string           engineInclude = "EngineInclude";

    std::vector<const char*> searchPaths;
    if (!shaderDirStr.empty())
        searchPaths.push_back(shaderDirStr.c_str());
    if (!shaderRootStr.empty())
        searchPaths.push_back(shaderRootStr.c_str());
    searchPaths.push_back(engineInclude.c_str());

    // ---- Preprocessor macros parsed from asset-key suffix. ----
    std::vector<slang::PreprocessorMacroDesc> slangMacros;
    slangMacros.reserve(macros.size());
    for (const auto& [name, value] : macros)
    {
        slang::PreprocessorMacroDesc m{};
        m.name  = name.c_str();
        m.value = value.c_str();
        slangMacros.push_back(m);
    }

    slang::SessionDesc sessionDesc{};
    sessionDesc.targets                 = &targetDesc;
    sessionDesc.targetCount             = 1;
    sessionDesc.searchPaths             = searchPaths.data();
    sessionDesc.searchPathCount         = (SlangInt)searchPaths.size();
    sessionDesc.preprocessorMacros      = slangMacros.empty() ? nullptr : slangMacros.data();
    sessionDesc.preprocessorMacroCount  = (SlangInt)slangMacros.size();
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR; // matches GLSL/Vulkan default

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(self.m_impl->globalSession->createSession(sessionDesc, session.writeRef())))
    {
        YG_CORE_ERROR("Slang: createSession failed for {0}", sourcePath.string());
        return {};
    }

    // ---- Load the source as an in-memory module. ----
    Slang::ComPtr<slang::IBlob> diagnostics;

    // Module name = file stem; helps Slang produce nicer error messages.
    std::string moduleName = sourcePath.stem().string();
    if (moduleName.empty())
        moduleName = "shader";
    std::string pathStr = sourcePath.string();

    slang::IModule* module = session->loadModuleFromSourceString(
        moduleName.c_str(), pathStr.c_str(), slangSource.c_str(), diagnostics.writeRef());
    if (!module)
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR(
                "Slang: load module '{0}' failed:\n{1}", pathStr, (const char*)diagnostics->getBufferPointer());
        }
        else
        {
            YG_CORE_ERROR("Slang: load module '{0}' failed (no diagnostics)", pathStr);
        }
        return {};
    }

    // ---- Find the requested entry point. ----
    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    if (SLANG_FAILED(module->findEntryPointByName(entryPoint.c_str(), entryPointObj.writeRef())))
    {
        // Some entry points in user code may not be tagged with [shader("...")]
        // - fall back to findAndCheckEntryPoint with an explicit stage so we
        // can pick a function by name+stage in that case.
        diagnostics.setNull();
        if (SLANG_FAILED(module->findAndCheckEntryPoint(
                entryPoint.c_str(), YgShaderStage2SlangStage(stage), entryPointObj.writeRef(), diagnostics.writeRef())))
        {
            if (diagnostics && diagnostics->getBufferSize() > 0)
            {
                YG_CORE_ERROR("Slang: entry point '{0}' not found in '{1}':\n{2}",
                              entryPoint,
                              pathStr,
                              (const char*)diagnostics->getBufferPointer());
            }
            else
            {
                YG_CORE_ERROR("Slang: entry point '{0}' not found in '{1}'", entryPoint, pathStr);
            }
            return {};
        }
    }

    // ---- Compose the program: module + entry point. ----
    slang::IComponentType*               components[] = { module, entryPointObj.get() };
    Slang::ComPtr<slang::IComponentType> program;
    diagnostics.setNull();
    if (SLANG_FAILED(session->createCompositeComponentType(components, 2, program.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: composite create failed for '{0}':\n{1}",
                          pathStr,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }

    // Linking fully resolves the program and runs final IR passes.
    Slang::ComPtr<slang::IComponentType> linkedProgram;
    diagnostics.setNull();
    if (SLANG_FAILED(program->link(linkedProgram.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: link failed for '{0}':\n{1}", pathStr, (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }
    if (diagnostics && diagnostics->getBufferSize() > 0)
    {
        // Linking can also emit warnings/notes - keep them visible without
        // failing the compile.
        YG_CORE_WARN("Slang: link diagnostics for '{0}':\n{1}", pathStr, (const char*)diagnostics->getBufferPointer());
    }

    // ---- Pull SPIR-V for entry point #0 (we only added one). ----
    Slang::ComPtr<slang::IBlob> spirvBlob;
    diagnostics.setNull();
    if (SLANG_FAILED(linkedProgram->getEntryPointCode(
            /*entryPointIndex*/ 0,
            /*targetIndex*/ 0,
            spirvBlob.writeRef(),
            diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: codegen failed for '{0}'::{1}:\n{2}",
                          pathStr,
                          entryPoint,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }

    if (!spirvBlob || spirvBlob->getBufferSize() == 0)
    {
        YG_CORE_ERROR("Slang: empty SPIR-V output for '{0}'::{1}", pathStr, entryPoint);
        return {};
    }

    std::vector<uint8_t> out(spirvBlob->getBufferSize());
    std::memcpy(out.data(), spirvBlob->getBufferPointer(), spirvBlob->getBufferSize());
    return out;
}

// ---- CompileSpecialized ---------------------------------------------------
//
// Generic-entry path. Same session setup as Compile(), plus:
//   1. resolve `materialTypeName` to a Slang ITypeReflection on the module,
//   2. wrap it in a SpecializationArg::Kind::Type,
//   3. call entryPoint->specialize(...) -> a new IComponentType,
//   4. composite (module + specializedEntry) -> link -> getEntryPointCode.
//
// Slang requires specialization to happen on the entry-point component
// before linking (specialize-after-link returns a bogus reflection); the
// order below mirrors what the IMaterial sample in the Slang docs does.
//
// Phase 3 only specialized fragment entries; Phase 5 generalises to task
// and mesh stages too so the entire (task, mesh, fragment) trio shares a
// single `M : IMaterial` parameterisation -- needed for material-driven
// vertex hooks that have to run in the mesh stage.

std::vector<uint8_t> SlangShaderCompiler::CompileSpecialized(
    const std::string&                                      slangSource,
    const std::filesystem::path&                            sourcePath,
    const std::string&                                      entryPoint,
    ShaderStage                                             stage,
    const std::string&                                      materialTypeName,
    const std::string&                                      shaderKey,
    const std::vector<std::pair<std::string, std::string>>& macros)
{
    auto& self = Get();
    if (!self.m_impl || !self.m_impl->globalSession)
    {
        YG_CORE_ERROR("Slang: global session not available");
        return {};
    }

    std::lock_guard<std::mutex> lock(self.m_impl->sessionMutex);

    slang::TargetDesc targetDesc{};
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = self.m_impl->globalSession->findProfile("spirv_1_5");
    targetDesc.flags   = SLANG_TARGET_FLAG_GENERATE_SPIRV_DIRECTLY;
    // targetDesc.forceGLSLScalarBufferLayout = true;

    // ---- Search paths: source's own directory + EngineAssets/Shaders root + "EngineInclude".
    std::filesystem::path shaderDir     = sourcePath.parent_path();
    std::string           shaderDirStr  = shaderDir.string();
    std::string           shaderRootStr = "EngineAssets/Shaders/";
    std::string           engineInclude = "EngineInclude";

    std::vector<const char*> searchPaths;
    if (!shaderDirStr.empty())
        searchPaths.push_back(shaderDirStr.c_str());
    searchPaths.push_back(shaderRootStr.c_str());
    searchPaths.push_back(engineInclude.c_str());

    std::vector<slang::PreprocessorMacroDesc> slangMacros;
    slangMacros.reserve(macros.size());
    for (const auto& [name, value] : macros)
    {
        slang::PreprocessorMacroDesc m{};
        m.name  = name.c_str();
        m.value = value.c_str();
        slangMacros.push_back(m);
    }

    slang::SessionDesc sessionDesc{};
    sessionDesc.targets                 = &targetDesc;
    sessionDesc.targetCount             = 1;
    sessionDesc.searchPaths             = searchPaths.data();
    sessionDesc.searchPathCount         = (SlangInt)searchPaths.size();
    sessionDesc.preprocessorMacros      = slangMacros.empty() ? nullptr : slangMacros.data();
    sessionDesc.preprocessorMacroCount  = (SlangInt)slangMacros.size();
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(self.m_impl->globalSession->createSession(sessionDesc, session.writeRef())))
    {
        YG_CORE_ERROR("Slang: createSession failed for {0}", sourcePath.string());
        return {};
    }

    Slang::ComPtr<slang::IBlob> diagnostics;

    std::string moduleName = sourcePath.stem().string();
    if (moduleName.empty())
        moduleName = "shader";
    std::string pathStr = sourcePath.string();

    slang::IModule* module = session->loadModuleFromSourceString(
        moduleName.c_str(), pathStr.c_str(), slangSource.c_str(), diagnostics.writeRef());
    if (!module)
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR(
                "Slang: load module '{0}' failed:\n{1}", pathStr, (const char*)diagnostics->getBufferPointer());
        }
        else
        {
            YG_CORE_ERROR("Slang: load module '{0}' failed (no diagnostics)", pathStr);
        }
        return {};
    }

    // Generic fragment entry. Slang lets us look it up by name even when
    // it has unspecialized type parameters; the resulting IComponentType
    // is what we specialize next.
    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    if (SLANG_FAILED(module->findEntryPointByName(entryPoint.c_str(), entryPointObj.writeRef())))
    {
        diagnostics.setNull();
        if (SLANG_FAILED(module->findAndCheckEntryPoint(
                entryPoint.c_str(), YgShaderStage2SlangStage(stage), entryPointObj.writeRef(), diagnostics.writeRef())))
        {
            if (diagnostics && diagnostics->getBufferSize() > 0)
            {
                YG_CORE_ERROR("Slang: entry point '{0}' not found in '{1}':\n{2}",
                              entryPoint,
                              pathStr,
                              (const char*)diagnostics->getBufferPointer());
            }
            else
            {
                YG_CORE_ERROR("Slang: entry point '{0}' not found in '{1}'", entryPoint, pathStr);
            }
            return {};
        }
    }

    // Resolve the material type. This walks every imported module Slang
    // pulled in via `import Materials.Standard;` etc., so the user only
    // needs to make sure the desired impl is imported (directly or
    // transitively) somewhere in slangSource.
    slang::TypeReflection* matType = module->getLayout()->findTypeByName(materialTypeName.c_str());
    if (!matType)
    {
        YG_CORE_ERROR("Slang: type '{0}' not found in module '{1}' (forgot an `import`?)", materialTypeName, pathStr);
        return {};
    }

    // One specialization argument: the IMaterial impl type.
    slang::SpecializationArg specArg{};
    specArg.kind = slang::SpecializationArg::Kind::Type;
    specArg.type = matType;

    Slang::ComPtr<slang::IComponentType> specializedEntry;
    diagnostics.setNull();
    if (SLANG_FAILED(entryPointObj->specialize(&specArg, 1, specializedEntry.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: specialize<{0}> failed for '{1}':\n{2}",
                          materialTypeName,
                          pathStr,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }

    // Composite: module + specialized-entry. Specialization happens BEFORE
    // link so reflection (Phase 4) sees the concrete type for codegen.
    slang::IComponentType*               components[] = { module, specializedEntry.get() };
    Slang::ComPtr<slang::IComponentType> program;
    diagnostics.setNull();
    if (SLANG_FAILED(session->createCompositeComponentType(components, 2, program.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: composite create failed for specialized '{0}':\n{1}",
                          pathStr,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    diagnostics.setNull();
    if (SLANG_FAILED(program->link(linkedProgram.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: link failed for specialized '{0}':\n{1}",
                          pathStr,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }
    if (diagnostics && diagnostics->getBufferSize() > 0)
    {
        YG_CORE_WARN("Slang: link diagnostics for specialized '{0}':\n{1}",
                     pathStr,
                     (const char*)diagnostics->getBufferPointer());
    }

    Slang::ComPtr<slang::IBlob> spirvBlob;
    diagnostics.setNull();
    if (SLANG_FAILED(linkedProgram->getEntryPointCode(
            /*entryPointIndex*/ 0, /*targetIndex*/ 0, spirvBlob.writeRef(), diagnostics.writeRef())))
    {
        if (diagnostics && diagnostics->getBufferSize() > 0)
        {
            YG_CORE_ERROR("Slang: codegen failed for specialized '{0}'::{1}<{2}>:\n{3}",
                          pathStr,
                          entryPoint,
                          materialTypeName,
                          (const char*)diagnostics->getBufferPointer());
        }
        return {};
    }

    if (!spirvBlob || spirvBlob->getBufferSize() == 0)
    {
        YG_CORE_ERROR(
            "Slang: empty SPIR-V output for specialized '{0}'::{1}<{2}>", pathStr, entryPoint, materialTypeName);
        return {};
    }

    std::vector<uint8_t> out(spirvBlob->getBufferSize());
    std::memcpy(out.data(), spirvBlob->getBufferPointer(), spirvBlob->getBufferSize());
    return out;
}

// ---- ReflectMaterialSchema -----------------------------------------------
//
// Extract MaterialSchema by compiling + linking a minimal Slang program
// that imports the material module and declares a dummy entry point.
// No SPIR-V is generated; we only need the linked program for reflection.
//
// The wrapper forces Slang to compute full type layouts (including uniform
// field offsets) which are not available from a module-only composite.
//
MaterialSchema SlangShaderCompiler::ReflectMaterialSchema(const std::string&           slangSource,
                                                          const std::filesystem::path& sourcePath,
                                                          const std::string&           materialTypeName)
{
    auto& self = Get();
    if (!self.m_impl || !self.m_impl->globalSession || materialTypeName.empty())
        return {};

    std::lock_guard<std::mutex> lock(self.m_impl->sessionMutex);

    // ---- Same target + search-path setup as Compile() ----
    slang::TargetDesc targetDesc{};
    targetDesc.format  = SLANG_SPIRV;
    targetDesc.profile = self.m_impl->globalSession->findProfile("spirv_1_5");

    std::filesystem::path shaderDir     = sourcePath.parent_path();
    std::string           shaderDirStr  = shaderDir.string();
    std::string           shaderRootStr = "EngineAssets/Shaders/";
    std::string           engineInclude = "EngineInclude";

    std::vector<const char*> searchPaths;
    if (!shaderDirStr.empty())
        searchPaths.push_back(shaderDirStr.c_str());
    searchPaths.push_back(shaderRootStr.c_str());
    searchPaths.push_back(engineInclude.c_str());

    slang::SessionDesc sessionDesc{};
    sessionDesc.targets                 = &targetDesc;
    sessionDesc.targetCount             = 1;
    sessionDesc.searchPaths             = searchPaths.data();
    sessionDesc.searchPathCount         = (SlangInt)searchPaths.size();
    sessionDesc.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR;

    Slang::ComPtr<slang::ISession> session;
    if (SLANG_FAILED(self.m_impl->globalSession->createSession(sessionDesc, session.writeRef())))
        return {};

    // ---- Load the material module ----
    std::string moduleName = sourcePath.stem().string();
    if (moduleName.empty())
        moduleName = "material";
    std::string pathStr = sourcePath.string();

    Slang::ComPtr<slang::IBlob> diagnostics;

    slang::IModule* matModule = session->loadModuleFromSourceString(
        moduleName.c_str(), pathStr.c_str(), slangSource.c_str(), diagnostics.writeRef());
    if (!matModule)
        return {};

    // ---- Create a minimal wrapper module with a dummy compute entry point.
    //     This forces Slang to produce a full program layout with uniform offsets.
    //
    //     The wrapper imports the material module and declares a trivial
    //     compute shader that does nothing -- we never generate code from it.
    //
    //     We use the module name to build the import statement.
    std::string wrapperSource = "import " + moduleName +
        ";\n"
        "[shader(\"compute\")]\n"
        "[numthreads(1,1,1)]\n"
        "void reflectEntry() {}\n";

    slang::IModule* wrapperModule = session->loadModuleFromSourceString(
        "__reflect_wrapper", "__reflect_wrapper.slang", wrapperSource.c_str(), diagnostics.writeRef());
    if (!wrapperModule)
        return {};

    // ---- Find the compute entry point ----
    Slang::ComPtr<slang::IEntryPoint> entryPointObj;
    if (SLANG_FAILED(wrapperModule->findEntryPointByName("reflectEntry", entryPointObj.writeRef())))
        return {};

    // ---- Composite (matModule + wrapperModule + entry) and link ----
    slang::IComponentType*               components[] = { matModule, wrapperModule, entryPointObj.get() };
    Slang::ComPtr<slang::IComponentType> program;
    diagnostics.setNull();
    if (SLANG_FAILED(session->createCompositeComponentType(components, 3, program.writeRef(), diagnostics.writeRef())))
        return {};

    Slang::ComPtr<slang::IComponentType> linkedProgram;
    if (SLANG_FAILED(program->link(linkedProgram.writeRef(), diagnostics.writeRef())))
        return {};

    // ---- Extract schema via reflection ----
    MaterialSchema schema;
    if (!BuildMaterialSchemaFromReflection(
            self.m_impl->globalSession.get(), linkedProgram.get(), materialTypeName, schema))
        return {};

    return schema;
}

SlangShaderCompiler* SlangShaderCompiler::s_instance = nullptr;

} // namespace Yogi
