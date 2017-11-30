set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

add_definitions(-DP2PSOCKET_LIBRARY)

add_library(p2psocket SHARED
    global.hpp
    p2psocket.cpp
    p2psocket.hpp)

target_link_libraries(p2psocket include socket)
