project( stb_image )

if ( BUILD_SHARED_LIBS )
    add_library( stb_image SHARED stb_image.cpp )
else()
    add_library( stb_image STATIC stb_image.cpp )
endif()

target_include_directories( stb_image PUBLIC . )