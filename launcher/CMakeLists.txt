project (Launcher)

add_compile_definitions(LAUNCHER_DIR=\"${CMAKE_CURRENT_SOURCE_DIR}/examples\")

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS
    src/*.cpp
    ../editor/src/reflect/*.cpp
    ../editor/src/script/*.cpp
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
elseif ( CMAKE_CXX_COMPILER_ID STREQUAL "GNU" )
    if ( WIN32 )
        target_compile_options(launcher PRIVATE -Wa,-mbig-obj)
    endif()
endif()