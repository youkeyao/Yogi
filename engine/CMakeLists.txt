project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    include_directories( vendor/glfw/include )
    add_subdirectory( vendor/glfw )
    file(GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            src/backends/window/glfw/*.cpp
    )
    set(yg_window glfw)
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    include_directories( vendor/glad/include )
    add_subdirectory( vendor/glad )
    file(GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/backends/renderer/opengl/*.cpp
    )
    set(yg_renderer glad)
elseif ( YG_RENDERER_API STREQUAL "Vulkan" )
    include_directories( vendor/VulkanSDK/include )
    file(GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/backends/renderer/vulkan/*.cpp
    )
    if (WIN32)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/Win32/vulkan-1.lib)
        add_compile_definitions(YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/Win32\")
    elseif (APPLE)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/MacOS/libvulkan.1.dylib)
        add_compile_definitions(YG_VK_LAYER_PATH=\"${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/bin/MacOS\")
    elseif (UNIX)
        set(yg_renderer ${CMAKE_CURRENT_SOURCE_DIR}/vendor/VulkanSDK/lib/Linux/libvulkan.so.1.3.239)
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
set(YG_BUILD_SHADER_FOLDER "build")
add_compile_definitions(YG_SHADER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/src/shaders/${YG_BUILD_SHADER_FOLDER}/\")

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
add_subdirectory( vendor/SPIRV-Cross )

file(GLOB_RECURSE
    YG_ENGINE_SOURCES CONFIGURE_DEPENDS
        # src/runtime/*.cpp
        src/runtime/core/*.cpp
        src/runtime/events/*.cpp
        src/runtime/scene/*.cpp
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
        stb_image
        spirv-cross-glsl
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
        vendor/glm
        vendor/entt/src
    PRIVATE
        vendor/SPIRV-Cross
        vendor/spdlog/include
        vendor/stb_image
)
add_dependencies(engine Shaders)