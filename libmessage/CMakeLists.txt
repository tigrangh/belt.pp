set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

add_definitions(-DMESSAGE_LIBRARY)

add_library(message SHARED
    global.hpp
    message.cpp
    message.hpp)

target_link_libraries(message include)
