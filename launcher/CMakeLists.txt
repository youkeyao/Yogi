project (Launcher)

add_compile_definitions(LAUNCHER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/examples\")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    ../editor/src/reflect/component_manager.cpp
    ../editor/src/reflect/system_manager.cpp
    ../editor/src/reflect/scene_manager.cpp
    ../editor/src/script/script_manager.cpp
)
add_executable(launcher ${SOURCES})
target_include_directories( launcher
    PRIVATE
        src
        ../editor/src
        ../editor/vendor/rapidjson/include
        ../editor/vendor/sol2/include
)
target_link_libraries( launcher
    PRIVATE
        engine
        libluajit
)
target_compile_definitions(launcher PRIVATE YG_SCRIPT_NO_SAFE)
if ( MSVC )
    target_compile_options(launcher PRIVATE /bigobj)
elseif ( CMAKE_COMPILER_IS_GNUCXX )
    target_compile_options(launcher PRIVATE -Wa,-mbig-obj)
endif()