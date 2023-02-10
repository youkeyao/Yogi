project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

set ( YG_WINDOW_API "GLFW" )
set ( YG_RENDERER_API "OpenGL" )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_definitions(-DYG_WINDOW_API=1)
    add_subdirectory( vendor/glfw )
    file(GLOB_RECURSE
        WINDOW_SOURCES CONFIGURE_DEPENDS
            src/platform/window/glfw/*.cpp
    )
    set(window glfw)
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_definitions(-DYG_RENDERER_API=1)
    add_subdirectory( vendor/glad )
    file(GLOB_RECURSE
        RENDERER_SOURCES CONFIGURE_DEPENDS
            src/platform/renderer/opengl/*.cpp
    )
    set(renderer glad)
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

# ---imgui----------------------------------------------------
file(GLOB
    IMGUI_SOURSES CONFIGURE_DEPENDS
        vendor/imgui/*.cpp
)
add_library( imgui
    STATIC
        ${IMGUI_SOURSES}
)
target_include_directories( imgui
    PRIVATE
        vendor/imgui
)
# -----------------------------------------------------------

add_subdirectory( vendor/spdlog )
add_subdirectory( vendor/stb_image )

file(GLOB_RECURSE
    SOURCES CONFIGURE_DEPENDS
        src/base/*.cpp
)

add_library( engine
    STATIC
        ${SOURCES}
        ${WINDOW_SOURCES}
        ${RENDERER_SOURCES}
)
target_link_libraries( engine
    PUBLIC
        spdlog
        imgui
        stb_image
        ${window}
        ${renderer}
)
target_precompile_headers( engine
    PUBLIC
        src/engine_pch.h
)
target_include_directories( engine
    PUBLIC
        src
        vendor/glm
        vendor/imgui
    PRIVATE
        vendor/glfw/include
        vendor/glad/include
        vendor/spdlog/include
        vendor/stb_image
)