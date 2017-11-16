set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

add_definitions(-DSOCKET_LIBRARY)

add_library(socket SHARED
    global.hpp
    socket.cpp
    socket.hpp)

target_link_libraries(socket include)
