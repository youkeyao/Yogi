cmake_minimum_required( VERSION 3.16 )
set( CMAKE_CXX_STANDARD 17 )

set( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_HOME_DIRECTORY}/bin )

project( Yogi )

add_subdirectory( engine )
add_subdirectory( sandbox )
add_subdirectory( editor )