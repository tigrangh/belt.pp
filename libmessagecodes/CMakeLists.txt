set( CMAKE_CXX_STANDARD 11 )
set( CMAKE_CXX_STANDARD_REQUIRED YES )

add_definitions(-DMESSAGECODES_LIBRARY)

add_library(messagecodes SHARED
    global.hpp
    messagecodes.cpp
    messagecodes.hpp)

target_link_libraries(messagecodes include)
