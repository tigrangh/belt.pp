cmake_minimum_required(VERSION 3.2)

project(belt.pp)

set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

add_compile_options(-Wall)
add_compile_options(-Werror)

add_subdirectory(src)
