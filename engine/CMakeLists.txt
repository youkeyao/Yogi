project( Engine )

set( GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE )
set( GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE )

add_subdirectory( vendor/glfw )
add_subdirectory( vendor/spdlog )

# source code
file(GLOB 
    SOURCES CONFIGURE_DEPENDS
        src/core/*.cpp
        src/platform/window/*.cpp
)

# library creation
add_library( engine
    STATIC
        ${SOURCES}
)
target_link_libraries( engine
    PUBLIC
        glfw
        spdlog
)
target_precompile_headers( engine
    PUBLIC
        src/engine_pch.h
)
target_include_directories( engine
    PUBLIC
        src
    PRIVATE
        vendor/glfw/include
        vendor/spdlog/include
)