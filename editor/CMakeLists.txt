project (Editor)

file(GLOB_RECURSE SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(editor ${SOURCES})
target_link_libraries( editor
    PRIVATE
        engine
)