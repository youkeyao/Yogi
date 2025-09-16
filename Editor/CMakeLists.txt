project( Editor )

file( GLOB_RECURSE YG_EDITOR_SOURCES CONFIGURE_DEPENDS Source/*.cpp )

# ---imgui----------------------------------------------------
file( GLOB
    IMGUI_SOURSES CONFIGURE_DEPENDS
        ThirdParty/imgui/*.cpp
)
list( APPEND YG_EDITOR_SOURCES ${IMGUI_SOURSES})
# -----------------------------------------------------------

# ---imguizmo----------------------------------------------------
file( GLOB
    IMGUIZMO_SOURSES CONFIGURE_DEPENDS
        ThirdParty/ImGuizmo/*.cpp
)
list( APPEND YG_EDITOR_SOURCES ${IMGUIZMO_SOURSES})
# -----------------------------------------------------------

add_executable( YGEditor ${YG_EDITOR_SOURCES} )
target_link_libraries( YGEditor
    PRIVATE
        YGEngine
)
if ( GLFW_BUILD_X11 )
    target_link_libraries( YGEditor PRIVATE
        X11
    )
endif()
target_include_directories( YGEditor
    PRIVATE
        Include
        ../Engine/Source
        ThirdParty/imgui
        ThirdParty/ImGuizmo
)

# copy imgui.ini
add_custom_command(
    TARGET YGEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/imgui.ini
)
# copy assets
add_custom_command(
    TARGET YGEditor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/Assets
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/Assets
)