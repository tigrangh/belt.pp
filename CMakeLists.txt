cmake_minimum_required(VERSION 3.2)

project(belt.pp)

#include_directories ("${PROJECT_SOURCE_DIR}/include")
#link_directories ("${CMAKE_BINARY_DIR}/bin")
set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-Wall)
add_compile_options(-Werror)

add_subdirectory(belt.pp)
add_subdirectory(src)
