project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_subdirectory( vendor/glfw )
    file(GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            src/backends/window/glfw/*.cpp
    )
    set(yg_window glfw)
    set(yg_window_include vendor/glfw/include)
elseif ( YG_WINDOW_API STREQUAL "SDL" )
    set(SDL_ALSA off)
    set(SDL_TEST off)
    add_subdirectory( vendor/SDL )
    file(GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            src/backends/window/sdl/*.cpp
    )
    set(yg_window SDL2::SDL2)
    set(yg_window_include vendor/SDL/include)
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_subdirectory( vendor/glad )
    file(GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/backends/renderer/opengl/*.cpp
    )
    set(yg_renderer glad)
    set(yg_renderer_include vendor/glad/include)
elseif ( YG_RENDERER_API STREQUAL "Vulkan" )
    file(GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/backends/renderer/vulkan/*.cpp
    )
    set(yg_renderer_include vendor/VulkanSDK/include)
    if (WIN32)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/Win32/vulkan-1.lib)
        add_compile_definitions(YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/Win32\")
    elseif (APPLE)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/MacOS/libvulkan.1.dylib)
        add_compile_definitions(YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/MacOS\")
    elseif (UNIX)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/Linux/libvulkan.so.1)
        add_compile_definitions(YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/Linux\")
    else()
        message(FATAL_ERROR "Unknown platform")
    endif()
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

# ---Shader compiler------------------------------------------------------------------------
if (WIN32)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/Win32/glslangValidator.exe)
elseif (APPLE)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/MacOS/glslangValidator)
elseif (UNIX)
    set(yg_glslang_bin ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/Linux/glslangValidator)
else()
    message(FATAL_ERROR "Unknown platform")
endif()
#-------------------------------------------------------------
set(YG_BUILD_SHADER_FOLDER "build_shader")
add_compile_definitions(YG_SHADER_DIR=\"${YG_BUILD_SHADER_FOLDER}/\")
add_compile_definitions(YG_ASSET_DIR=\"assets/\")

add_subdirectory( src/shaders )
add_subdirectory( vendor/spdlog )
add_subdirectory( vendor/stb_image )
set(SPIRV_CROSS_CLI OFF)
set(SPIRV_CROSS_ENABLE_TESTS OFF)
set(SPIRV_CROSS_ENABLE_HLSL OFF)
set(SPIRV_CROSS_ENABLE_MSL OFF)
set(SPIRV_CROSS_ENABLE_CPP OFF)
set(SPIRV_CROSS_ENABLE_REFLECT OFF)
set(SPIRV_CROSS_ENABLE_C_API OFF)
set(SPIRV_CROSS_ENABLE_UTIL OFF)
set(SPIRV_CROSS_SKIP_INSTALL ON)
add_subdirectory( vendor/SPIRV-Cross )
set(ASSIMP_INSTALL OFF)
set(ASSIMP_BUILD_TESTS OFF)
add_subdirectory( vendor/assimp )
add_subdirectory( vendor/JoltPhysics/Build )

file(GLOB_RECURSE
    YG_ENGINE_SOURCES CONFIGURE_DEPENDS
        src/runtime/*.cpp
)

add_library( engine
    STATIC
        ${YG_ENGINE_SOURCES}
        ${YG_WINDOW_SOURCES}
        ${YG_RENDERER_SOURCES}
)
target_link_libraries( engine
    PUBLIC
        spdlog
        Jolt
    PRIVATE
        stb_image
        spirv-cross-glsl
        assimp
        ${yg_window}
        ${yg_renderer}
)
target_precompile_headers( engine
    PUBLIC
        src/engine_pch.h
)
target_include_directories( engine
    PUBLIC
        src
        ${yg_window_include}
        ${yg_renderer_include}
        vendor/glm
        vendor/entt/src
        vendor/JoltPhysics
        vendor/SPIRV-Cross
        vendor/spdlog/include
        vendor/stb_image
        vendor/assimp/include
)
add_dependencies(engine Shaders)

# copy assets
add_custom_command(
    TARGET engine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)