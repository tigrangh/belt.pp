cmake_minimum_required(VERSION 3.2)

project(belt.pp)

include_directories ("${PROJECT_SOURCE_DIR}/include")
#link_directories ("${CMAKE_BINARY_DIR}/bin")

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_compile_options(-Wall)
add_compile_options(-Werror)

add_subdirectory(include)
add_subdirectory(libprocessor)
add_subdirectory(test_async)
add_subdirectory(test_cp)
add_subdirectory(test_cpdummy)
