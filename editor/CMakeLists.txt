project (Editor)

file(GLOB_RECURSE
    SOURCES CONFIGURE_DEPENDS
        src/*.cpp
)

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_compile_definitions(YG_WINDOW_API=1)
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_compile_definitions(YG_RENDERER_API=1)
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

add_executable(editor ${SOURCES})
target_link_libraries( editor
    PRIVATE
        engine
        imgui
)
target_include_directories( editor
    PRIVATE
        src
        vendor/imgui
)