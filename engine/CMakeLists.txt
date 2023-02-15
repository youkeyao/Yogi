project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_compile_definitions(YG_WINDOW_API=1)
    add_subdirectory( vendor/glfw )
    file(GLOB_RECURSE
        YG_WINDOW_SOURCES CONFIGURE_DEPENDS
            src/platform/window/glfw/*.cpp
    )
    set(yg_window glfw)
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_compile_definitions(YG_RENDERER_API=1)
    add_subdirectory( vendor/glad )
    file(GLOB_RECURSE
        YG_RENDERER_SOURCES CONFIGURE_DEPENDS
            src/platform/renderer/opengl/*.cpp
    )
    set(yg_renderer glad)
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

add_subdirectory( vendor/spdlog )
add_subdirectory( vendor/stb_image )

file(GLOB_RECURSE
    YG_ENGINE_SOURCES CONFIGURE_DEPENDS
        src/base/*.cpp
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
    PRIVATE
        vendor/glfw/include
        vendor/glad/include
        vendor/spdlog/include
        vendor/stb_image
)