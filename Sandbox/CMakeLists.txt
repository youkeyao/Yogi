project( Sandbox )

file( GLOB_RECURSE SOURCES CONFIGURE_DEPENDS Source/*.cpp )
add_executable( Sandbox ${SOURCES} )
target_link_libraries( Sandbox
    PRIVATE
        YGEngine
)
target_include_directories( Sandbox
    PRIVATE
        Include
)