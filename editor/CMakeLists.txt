project (Editor)

file(GLOB_RECURSE YG_EDITOR_SOURCES CONFIGURE_DEPENDS src/*.cpp)

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

# ---imguizmo----------------------------------------------------
file(GLOB
    IMGUIZMO_SOURSES CONFIGURE_DEPENDS
        vendor/ImGuizmo/*.cpp
)
add_library( imguizmo
    STATIC
        ${IMGUIZMO_SOURSES}
)
target_include_directories( imguizmo
    PRIVATE
        vendor/imgui
        vendor/ImGuizmo
)
# -----------------------------------------------------------

# ---luajit----------------------------------------------------
set( LUAJIT_BUILD_TOOL OFF )
add_subdirectory( vendor/luajit )
# -----------------------------------------------------------

add_compile_definitions(YG_EDITOR_ASSET_DIR=\"assets/\")
add_compile_definitions(YG_PROJECT_TEMPLATE=\"${CMAKE_SOURCE_DIR}/launcher/examples/template\")
add_executable(editor ${YG_EDITOR_SOURCES})
target_link_libraries( editor
    PUBLIC
        engine
        imgui
        imguizmo
        libluajit
)
target_include_directories( editor
    PRIVATE
        src
        vendor/imgui
        vendor/rapidjson/include
        vendor/portable-file-dialogs
        vendor/ImGuizmo
        vendor/sol2/include
)
if ( MSVC )
    target_compile_options(editor PRIVATE /bigobj)
elseif ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(editor PRIVATE -Wa,-mbig-obj)
endif()

# copy imgui.ini
add_custom_command(
    TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/imgui.ini
)
# copy assets
add_custom_command(
    TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/assets
)