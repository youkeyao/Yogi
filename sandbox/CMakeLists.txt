project (Sandbox)

file(GLOB SOURCES src/*.cpp)
add_executable(sandbox ${SOURCES})
target_link_libraries( sandbox
    PRIVATE
        engine
)