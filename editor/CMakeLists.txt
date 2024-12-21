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

add_compile_definitions(YG_EDITOR_ASSETS=\"${CMAKE_CURRENT_SOURCE_DIR}/assets/\")
add_compile_definitions(YG_PROJECT_TEMPLATE=\"${CMAKE_CURRENT_SOURCE_DIR}/template\")
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
        ../engine/src
        ../engine/vendor/SPIRV-Cross
        ../engine/vendor/glfw/include
        ../engine/vendor/SDL/include
        ../engine/vendor/VulkanSDK/include
)

add_custom_command(
    TARGET editor POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${CMAKE_CURRENT_SOURCE_DIR}/imgui.ini
        ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/imgui.ini
)