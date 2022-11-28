project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

add_subdirectory( vendor/glfw )
add_subdirectory( vendor/glad )
add_subdirectory( vendor/spdlog )

# source code
file(GLOB_RECURSE
    SOURCES CONFIGURE_DEPENDS
        src/*.cpp
        vendor/imgui/backends/imgui_impl_opengl3.cpp
        vendor/imgui/backends/imgui_impl_glfw.cpp
)
file(GLOB
    IMGUI_SOURSES
        vendor/imgui/*.cpp
)

# library creation
add_library( engine
    STATIC
        ${SOURCES}
)
add_library( imgui
    STATIC
        ${IMGUI_SOURSES}
)
target_link_libraries( engine
    PUBLIC
        glfw
        glad
        spdlog
        imgui
)
target_precompile_headers( engine
    PUBLIC
        src/engine_pch.h
)
target_include_directories( engine
    PUBLIC
        src
        vendor/imgui
    PRIVATE
        vendor/glfw/include
        vendor/glad/include
        vendor/spdlog/include
)