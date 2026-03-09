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
    elseif ( APPLE )
        add_compile_definitions( VK_USE_PLATFORM_METAL_EXT )
    elseif ( UNIX )
        add_compile_definitions( VK_USE_PLATFORM_WAYLAND_KHR )
        add_compile_definitions( VK_USE_PLATFORM_XCB_KHR )
        add_compile_definitions( VK_USE_PLATFORM_XLIB_KHR )
    else()
        message( FATAL_ERROR "Unknown platform" )
    endif()
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

add_subdirectory( ThirdParty )

file( GLOB_RECURSE
    YG_ENGINE_SOURCES CONFIGURE_DEPENDS
        Source/Runtime/*.cpp
)

if ( BUILD_SHARED_LIBS )
    add_library( YGEngine
        SHARED
            ${YG_ENGINE_SOURCES}
            ${YG_WINDOW_SOURCES}
            ${YG_RENDERER_SOURCES}
    )
    if ( WIN32 )
        target_compile_definitions( YGEngine PUBLIC YG_SHARED_LIB )
        target_compile_definitions( YGEngine PRIVATE YG_EXPORT )
    endif()
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
        assimp
        # Jolt
        ${YGWindowLib}
        ${YGRendererLib}
    PRIVATE
        stb_image
        meshoptimizer
        glslang
        glslang::glslang-default-resource-limits
        # spirv-cross-glsl
)
target_precompile_headers( YGEngine
    PUBLIC
        Include/PCH.h
)
target_include_directories( YGEngine
    PUBLIC
        Include
        ThirdParty/glm
        ThirdParty/entt/src
        ThirdParty/pfr/include
    PRIVATE
        ${YGWindowInclude}
        ${YGRendererInclude}
        ThirdParty/zpp_bits
        # ThirdParty/JoltPhysics
        # ThirdParty/SPIRV-Cross
        # ThirdParty/stb_image
        # ThirdParty/assimp/include
)

# copy assets
add_custom_command(
    TARGET YGEngine POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/EngineAssets
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Include
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/EngineInclude
)