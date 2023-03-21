cmake_minimum_required( VERSION 3.16 )
set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

project( Yogi )

set ( YG_WINDOW_GLFW 1 )
set ( YG_WINDOW_SDL 2 )
set ( YG_RENDERER_OPENGL 1 )
set ( YG_RENDERER_VULKAN 2 )

# ---platform------------------------------------
if (WIN32)
    add_compile_definitions(YG_PLATFORM_WINDOWS)
elseif (APPLE)
    add_compile_definitions(YG_PLATFORM_APPLE)
elseif (UNIX)
    add_compile_definitions(YG_PLATFORM_UNIX)
else()
    message(FATAL_ERROR "Unknown platform")
endif()
#------------------------------------------------

set ( YG_WINDOW_API "GLFW" )
set ( YG_RENDERER_API "Vulkan" )

# ---window--------------------------------------------
if ( YG_WINDOW_API STREQUAL "GLFW" )
    add_compile_definitions(YG_WINDOW_API=${YG_WINDOW_GLFW})
elseif ( YG_WINDOW_API STREQUAL "SDL" )
    add_compile_definitions(YG_WINDOW_API=${YG_WINDOW_SDL})
else()
    message( FATAL_ERROR "Unknow window" )
endif()
#-------------------------------------------------------------

# ---renderer--------------------------------------------
if ( YG_RENDERER_API STREQUAL "OpenGL" )
    add_compile_definitions(YG_RENDERER_API=${YG_RENDERER_OPENGL})
elseif ( YG_RENDERER_API STREQUAL "Vulkan" )
    add_compile_definitions(YG_RENDERER_API=${YG_RENDERER_VULKAN})
else()
    message( FATAL_ERROR "Unknow renderer" )
endif()
#-------------------------------------------------------------

if ( CMAKE_BUILD_TYPE STREQUAL "Debug" )
    add_compile_definitions(YG_DEBUG)
endif()

add_subdirectory( engine )
add_subdirectory( sandbox )
add_subdirectory( editor )