cmake_minimum_required( VERSION 3.16 )
cmake_policy( SET CMP0077 NEW )

project( Yogi )
set( CMAKE_CXX_STANDARD 20 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${PROJECT_BINARY_DIR}/bin )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )
set( CMAKE_RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_RUNTIME_OUTPUT_DIRECTORY} )

# ---platform------------------------------------
if ( WIN32 )
    add_compile_definitions( YG_PLATFORM_WIN32 )
elseif ( APPLE )
    add_compile_definitions( YG_PLATFORM_MACOS )
elseif ( UNIX )
    add_compile_definitions( YG_PLATFORM_LINUX )
else()
    message( FATAL_ERROR "Unknown platform" )
endif()
#------------------------------------------------

set( YG_WINDOW_API "GLFW" )
set( YG_RENDERER_API "Vulkan" )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_compile_definitions( YG_WINDOW_GLFW )
elseif ( YG_WINDOW_API STREQUAL "SDL" )
    add_compile_definitions( YG_WINDOW_SDL )
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_compile_definitions( YG_RENDERER_OPENGL )
elseif ( YG_RENDERER_API STREQUAL "Vulkan" )
    add_compile_definitions( YG_RENDERER_VULKAN )
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    add_compile_definitions( YG_DEBUG )
endif()

set( BUILD_SHARED_LIBS OFF )

add_subdirectory( Engine )
add_subdirectory( Sandbox )