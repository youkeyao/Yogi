project (Sandbox)

file(GLOB sandbox_sources src/*.cpp)
add_executable(sandbox ${sandbox_sources})
target_link_libraries(sandbox
    PRIVATE
        engine
)