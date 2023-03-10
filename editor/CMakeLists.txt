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

add_executable(editor ${YG_EDITOR_SOURCES})
target_link_libraries( editor
    PRIVATE
        engine
        imgui
        imguizmo
)
target_include_directories( editor
    PRIVATE
        src
        vendor/imgui
        vendor/rapidjson/include
        vendor/portable-file-dialogs
        vendor/ImGuizmo
)