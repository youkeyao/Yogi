project (Sandbox)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(sandbox ${SOURCES})
target_link_libraries( sandbox
    PRIVATE
        engine
)