project( Engine )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
    set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
    set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )
    add_subdirectory( ThirdParty/glfw )
    file( GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            Source/Backends/Window/GLFW/*.cpp
    )
    set( YGWindowLib glfw )
    set( YGWindowInclude ThirdParty/glfw/include )
elseif ( YG_WINDOW_API STREQUAL "SDL" )
    set( SDL_ALSA off )
    set( SDL_TEST off )
    add_subdirectory( ThirdParty/SDL )
    file( GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            src/backends/window/sdl/*.cpp
    )
    set( yg_window SDL2::SDL2 )
    set( yg_window_include ThirdParty/SDL/include )
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_subdirectory( vendor/glad )
    file( GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/backends/renderer/opengl/*.cpp
    )
    set( YGRendererLib glad )
    set( YGRendererInclude vendor/glad/include )
elseif ( YG_RENDERER_API STREQUAL "Vulkan" )
    add_subdirectory( ThirdParty/Vulkan-Headers )
    set( VOLK_PULL_IN_VULKAN OFF )
    add_subdirectory( ThirdParty/volk )
    target_link_libraries(volk PRIVATE Vulkan::Headers)
    file( GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            Source/Backends/Renderer/Vulkan/*.cpp
    )
    set( YGRendererLib volk Vulkan::Headers )
    set( YGRendererInclude ThirdParty/volk )
    if ( WIN32 )
        add_compile_definitions( VK_USE_PLATFORM_WIN32_KHR )
        add_compile_definitions( YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/Win32\" )
    elseif ( APPLE )
        add_compile_definitions( VK_USE_PLATFORM_METAL_EXT )
        add_compile_definitions( YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/MacOS\" )
    elseif ( UNIX )
        add_compile_definitions( VK_USE_PLATFORM_WAYLAND_KHR )
        add_compile_definitions( VK_USE_PLATFORM_XCB_KHR )
        add_compile_definitions( VK_USE_PLATFORM_XLIB_KHR )
        add_compile_definitions( YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/Linux\" )
    else()
        message( FATAL_ERROR "Unknown platform" )
    endif()
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

# ---Shader compiler------------------------------------------------------------------------
if (WIN32)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/Win32/glslangValidator.exe)
elseif (APPLE)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/MacOS/glslangValidator)
elseif (UNIX)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/ThirdParty/VulkanLayer/Linux/glslangValidator)
else()
    message(FATAL_ERROR "Unknown platform")
endif()
#-------------------------------------------------------------
set( YG_BUILD_SHADER_FOLDER "BuildShader" )
add_compile_definitions( YG_SHADER_DIR=\"${YG_BUILD_SHADER_FOLDER}/\" )
add_compile_definitions( YG_ASSET_DIR=\"Assets/\" )

add_subdirectory( Source/Shaders )
add_subdirectory( ThirdParty/spdlog )
add_subdirectory( ThirdParty/stb_image )
# set(SPIRV_CROSS_CLI OFF)
# set(SPIRV_CROSS_ENABLE_TESTS OFF)
# set(SPIRV_CROSS_ENABLE_HLSL OFF)
# set(SPIRV_CROSS_ENABLE_MSL OFF)
# set(SPIRV_CROSS_ENABLE_CPP OFF)
# set(SPIRV_CROSS_ENABLE_REFLECT OFF)
# set(SPIRV_CROSS_ENABLE_C_API OFF)
# set(SPIRV_CROSS_ENABLE_UTIL OFF)
# set(SPIRV_CROSS_SKIP_INSTALL ON)
# add_subdirectory( vendor/SPIRV-Cross )
# set(ASSIMP_INSTALL OFF)
# set(ASSIMP_BUILD_TESTS OFF)
# add_subdirectory( vendor/assimp )
# add_subdirectory( vendor/JoltPhysics/Build )

file( GLOB_RECURSE
    YG_ENGINE_SOURCES CONFIGURE_DEPENDS
        Source/Runtime/*.cpp
)

if (BUILD_SHARED_LIBS)
    add_library( YGEngine
        SHARED
            ${YG_ENGINE_SOURCES}
            ${YG_WINDOW_SOURCES}
            ${YG_RENDERER_SOURCES}
    )
    target_compile_definitions( YGEngine PUBLIC YG_SHARED_LIB )
else()
    add_library( YGEngine
        STATIC
            ${YG_ENGINE_SOURCES}
            ${YG_WINDOW_SOURCES}
            ${YG_RENDERER_SOURCES}
    )
endif()
target_link_libraries( YGEngine
    PUBLIC
        spdlog
        # Jolt
    PRIVATE
        stb_image
        # spirv-cross-glsl
        # assimp
        ${YGWindowLib}
        ${YGRendererLib}
)
target_precompile_headers( YGEngine
    PUBLIC
        Include/PCH.h
)
target_include_directories( YGEngine
    PUBLIC
        Include
        ThirdParty/spdlog/include
        ThirdParty/glm
    PRIVATE
        ${YGWindowInclude}
        ${YGRendererInclude}
        ThirdParty/entt/src
        # ThirdParty/JoltPhysics
        # ThirdParty/SPIRV-Cross
        ThirdParty/stb_image
        # ThirdParty/assimp/include
)
target_compile_definitions( YGEngine
    PRIVATE
        YG_EXPORT
)
add_dependencies(YGEngine Shaders)

# copy assets
add_custom_command(
    TARGET YGEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets
)