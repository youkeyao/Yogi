project( GLad )

if ( BUILD_SHARED_LIBS )
    add_library( glad SHARED src/glad.c )
else()
    add_library( glad STATIC src/glad.c )
endif()
target_include_directories( glad PUBLIC include )