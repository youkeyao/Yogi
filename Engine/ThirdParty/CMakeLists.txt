add_subdirectory( spdlog )
add_subdirectory( stb_image )
add_subdirectory( meshoptimizer )

# ---------- Slang (and its vendored shader-toolchain deps) ----------------
# Slang vendors its own copies of Vulkan-Headers, SPIRV-Headers, SPIRV-Tools,
# and glslang under slang/external/. We let Slang own all four for the whole
# build (the project-level Engine/ThirdParty/Vulkan-Headers / SPIRV-Headers /
# SPIRV-Tools / glslang submodules conflict with Slang's vendored copies via
# duplicate CMake target names: `Vulkan::Headers`, `SPIRV-Tools-opt`,
# `glslang::glslang`, etc.).
#
# CRITICAL: SLANG_ENABLE_SPIRV_TOOLS_MIMALLOC defaults to ON on Windows.
# That builds mimalloc and links it into slang-compiler.dll. mimalloc on
# Windows hooks malloc/free/operator new/operator delete process-wide via
# its `mi_override` mechanism, which corrupts the heap state of any module
# (Sandbox.exe, YGEngined.dll) that allocated through the original CRT
# heap before mimalloc loaded. The visible symptom is RtlValidateHeap
# failing inside ~vector<uint8_t> at random destruction sites that have
# nothing to do with Slang. We force it OFF to keep all modules on a
# single ucrtbased heap. SPIRV-Tools without mimalloc is just slightly
# slower at compile-time, no functional impact.
set( SLANG_ENABLE_GFX                 OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANG_RHI           OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_TESTS               OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_EXAMPLES            OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANGD              OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANGC              OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANGI              OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANGRT             OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_REPLAYER            OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_DXIL                OFF CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SLANG_GLSLANG       ON  CACHE BOOL "" FORCE )
set( SLANG_ENABLE_SPIRV_TOOLS_MIMALLOC OFF CACHE BOOL "" FORCE )
set( SLANG_SLANG_LLVM_FLAVOR          DISABLE CACHE STRING "" FORCE )
set( SLANG_LIB_TYPE                   SHARED  CACHE STRING "" FORCE )

# Local /utf-8 swap for the slang subtree only (Slang sources expect /utf-8,
# the project default of /source-charset:utf-8 conflicts via D8016).
get_directory_property( _yogi_inherited_compile_options COMPILE_OPTIONS )
set_directory_properties( PROPERTIES COMPILE_OPTIONS "" )
add_compile_options( $<$<CXX_COMPILER_ID:MSVC>:/utf-8> )
add_subdirectory( slang )
set_directory_properties( PROPERTIES COMPILE_OPTIONS "${_yogi_inherited_compile_options}" )
# --------------------------------------------------------------------------

set( ASSIMP_INSTALL OFF )
set( ASSIMP_BUILD_TESTS OFF )
set( ASSIMP_BUILD_ZLIB ON )
add_subdirectory( assimp )
if ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    target_compile_options( assimp PRIVATE -Wno-array-bounds )
endif()
# add_subdirectory( vendor/JoltPhysics/Build )