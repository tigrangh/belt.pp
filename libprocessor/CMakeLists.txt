set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )
add_definitions(-DPROCESSOR_LIBRARY)

add_library(processor SHARED
    processor.cpp
    processor.hpp
    global.hpp)
